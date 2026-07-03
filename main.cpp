#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <chrono>
#include <atomic>

#include "src/requests/requests.hpp"
#include "src/responses/responses.hpp"
#include "src/validator/validator.hpp"
#include "src/router/router.hpp"
#include "src/handler/handler.hpp"

using std::cout;

const int chunk_size = 8;

std::queue<std::string> lines_queue;
std::mutex queue_mutex;
std::condition_variable cv;
bool parsing_finished = false;

enum class ParseState
{
    RequestLine,
    Headers,
    Body
};

// Helper function to handle the HTTP parsing state machine.
// Returns true when a complete HTTP request has been fully parsed.
bool parse_http_line(const std::string &line, request &req, ParseState &state, size_t &expected_content_length)
{
    if (state == ParseState::RequestLine)
    {
        if (!pars_request_line(line, req))
        {
            std::cerr << "Failed to parse request line: " << line << "\n";
        }
        state = ParseState::Headers;
        return false;
    }

    if (state == ParseState::Headers)
    {
        if (line == "\r\n" || line == "")
        {
            // End of headers reached. Inspect Content-Length for a body.
            expected_content_length = 0;
            for (const auto &h : req.header)
            {
                std::string key_lower = h.key;
                for (char &c : key_lower)
                {
                    c = std::tolower(static_cast<unsigned char>(c));
                }

                if (key_lower == "content-length")
                {
                    expected_content_length = std::stoll(h.value);
                    cout << "Expected Content-Length: " << expected_content_length << "\n";
                    break;
                }
            }

            if (expected_content_length == 0)
            {
                req.body = "[LOG]: NO LOG";
                return true; // No body expected, request is complete!
            }
            state = ParseState::Body;
            return false;
        }
        else
        {
            if (!pars_header_line(line, req))
            {
                std::cerr << "Failed to parse header line: " << line << "\n";
            }
            return false;
        }
    }

    if (state == ParseState::Body)
    {
        req.body += line;
        if (req.body.length() >= expected_content_length)
        {
            if (req.body.length() > expected_content_length)
            {
                req.body = req.body.substr(0, expected_content_length);
            }
            return true; // Body fully read, request is complete!
        }
    }

    return false;
}

// Thread worker: Reads raw data from descriptor and breaks it into lines
void pars_fd_to_queue(int fd)
{
    char buffer[chunk_size];
    std::string sentence;
    ssize_t bytesRead = 0;

    while ((bytesRead = read(fd, buffer, chunk_size)) > 0)
    {
        for (int i = 0; i < bytesRead; i++)
        {
            sentence += buffer[i];
            if (buffer[i] == '\n')
            {
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    lines_queue.push(sentence);
                }
                cv.notify_one();
                sentence.clear();
            }
        }
        std::memset(buffer, 0, sizeof(buffer));
    }

    if (!sentence.empty())
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        lines_queue.push(sentence);
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        parsing_finished = true;
    }
    cv.notify_all();
}

int get_socket_server()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(42069);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cerr << "Error binding socket\n";
        close(fd);
        return -1;
    }
    if (listen(fd, 5) < 0)
    {
        std::cerr << "Error listening on socket\n";
        close(fd);
        return -1;
    }

    return fd;
}

int main()
{
    int serverFd = get_socket_server();
    if (serverFd < 0)
        return 1;

    int clientSocket = accept(serverFd, nullptr, nullptr);
    if (clientSocket < 0)
    {
        std::cerr << "Error accepting client connection\n";
        close(serverFd);
        return 1;
    }

    routing_table router;
    router[{"GET", "/"}] = handle_homepage;
    router[{"GET", "/api/data"}] = handle_api_data;

    // Spin up the producer thread to read from the socket
    std::thread producer(pars_fd_to_queue, clientSocket);

    // Parser State variables
    ParseState parse_state = ParseState::RequestLine;
    request req;
    size_t expected_content_length = 0;

    // Consumer Loop
    while (true)
    {
        std::string line;

        // Critical Section: Protect queue retrieval only
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, []
                    { return !lines_queue.empty() || parsing_finished; });

            // Safe exit condition: producer is done and queue is entirely drained
            if (lines_queue.empty() && parsing_finished)
            {
                break;
            }

            line = lines_queue.front();
            lines_queue.pop();
        }

        // Heavy lifting happens safely outside the lock
        bool request_complete = parse_http_line(line, req, parse_state, expected_content_length);

        if (request_complete)
        {
            print_request(req);
            response resp;

            // 1. Validation
            if (!validateRequest(req))
            {
                std::cerr << "Request is invalid\n";
                resp.statusCode = 400;
                resp.statusText = "Bad Request";
                resp.body = "<h1>400 - Bad Request</h1>";
            }
            else
            {
                // 2. Execution & Response (Triggered ONLY when request is fully formed)
                routing_key current_key = {req.method, req.uri};
                auto route_iterator = router.find(current_key);
                if (route_iterator != router.end())
                {
                    resp = route_iterator->second(req);
                }
                else
                {
                    resp = handle_404(req);
                }
            }

            sendResponse(clientSocket, resp);

            // 3. Reset pipeline variables for the next potential HTTP request
            parse_state = ParseState::RequestLine;
            req = request();
            expected_content_length = 0;
        }
    }

    if (producer.joinable())
    {
        producer.join();
    }

    close(clientSocket);
    close(serverFd);
    cout << "Server shutting down cleanly.\n";
    return 0;
}