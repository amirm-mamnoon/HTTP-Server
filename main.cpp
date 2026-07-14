#include <sys/socket.h>
#include <sys/time.h>
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
#include "src/handler/file_handler.hpp"

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"

using std::cout;

const int chunk_size = 8;
const int max_content_length = 10 * 1024 * 1024; // 10MB
const int max_line_length = 8192;

struct ClientState
{
    std::queue<std::string> lines_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool parsing_finished = false;
};

enum class ParseState
{
    RequestLine,
    Headers,
    Body,
    Error
};

bool parse_http_line(const std::string &line, request &req, ParseState &state, size_t &expected_content_length)
{
    if (state == ParseState::RequestLine)
    {
        if (!pars_request_line(line, req))
        {
            std::cerr << RED << BOLD << "✖ [ERROR] Failed to parse request line: " << RESET << line << "\n";
        }
        state = ParseState::Headers;
        return false;
    }

    if (state == ParseState::Headers)
    {
        if (line == "\r\n" || line == "")
        {
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
                    try
                    {
                        long long content_len = std::stoll(h.value);
                        if (content_len < 0)
                        {
                            std::cerr << "Invalid Content-Length (negative): " << content_len << '\n';
                            state = ParseState::Error;
                            return false;
                        }
                        if (content_len > max_content_length)
                        {
                            std::cerr << "Invalid Content-Length (10MG <): " << content_len << '\n';
                            state = ParseState::Error;
                            return false;
                        }
                        expected_content_length = static_cast<size_t>(content_len);
                    }
                    catch (const std::out_of_range &e)
                    {
                        std::cerr << "Value too large: " << e.what() << '\n';
                        state = ParseState::Error;
                        return false;
                    }
                    catch (const std::invalid_argument &e)
                    {
                        std::cerr << "Invalid format: " << e.what() << '\n';
                        state = ParseState::Error;
                        return false;
                    }

                    cout << MAGENTA << "⚡ [TRACE] Expected Content-Length: " << expected_content_length << RESET << "\n";
                    break;
                }
            }

            if (expected_content_length == 0)
            {
                req.body = "[LOG]: NO LOG";
                return true;
            }
            state = ParseState::Body;
            return false;
        }
        else
        {
            if (!pars_header_line(line, req))
            {
                std::cerr << RED << BOLD << "✖ [ERROR] Failed to parse header line: " << RESET << line << "\n";
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
            return true;
        }
    }

    return false;
}

void pars_fd_to_queue(int fd, ClientState &state)
{
    char buffer[chunk_size];
    std::string sentence;
    ssize_t bytesRead = 0;
    bool max_length_exceeded = false;

    while ((bytesRead = read(fd, buffer, chunk_size)) > 0 && !max_length_exceeded)
    {
        for (int i = 0; i < bytesRead; i++)
        {
            sentence += buffer[i];
            if (buffer[i] == '\n')
            {
                {
                    std::lock_guard<std::mutex> lock(state.queue_mutex);
                    state.lines_queue.push(sentence);
                }
                state.cv.notify_one();
                sentence.clear();
            } else if (sentence.length() >= 8192) {
                std::cerr << RED << "✖ [ERROR] Line exceeded max_line_length (" << max_line_length << ")\n";
                max_length_exceeded = true;
                sentence.clear();
                break;
            }
        }
        std::memset(buffer, 0, sizeof(buffer));
    }

    if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        std::cerr << RED << "⏳ [TIMEOUT] Client reading timed out. Dropping connection." << RESET << "\n";
    }

    if (!sentence.empty() && !max_length_exceeded)
    {
        std::lock_guard<std::mutex> lock(state.queue_mutex);
        state.lines_queue.push(sentence);
    }

    {
        std::lock_guard<std::mutex> lock(state.queue_mutex);
        state.parsing_finished = true;
    }
    state.cv.notify_all();
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
        std::cerr << RED << BOLD << "✖ [FATAL] Error binding socket\n"
                  << RESET;
        close(fd);
        return -1;
    }
    if (listen(fd, 5) < 0)
    {
        std::cerr << RED << BOLD << "✖ [FATAL] Error listening on socket\n"
                  << RESET;
        close(fd);
        return -1;
    }

    return fd;
}

void handle_client(int clientsocket, const routing_table &router)
{
    ClientState state;
    ParseState parse_state = ParseState::RequestLine;
    request req;
    size_t expected_content_length = 0;

    std::thread producer(pars_fd_to_queue, clientsocket, std::ref(state));

    while (true)
    {
        std::string line;

        {
            std::unique_lock<std::mutex> lock(state.queue_mutex);
            state.cv.wait(lock, [&state]
                          { return !state.lines_queue.empty() || state.parsing_finished; });

            if (state.lines_queue.empty() && state.parsing_finished)
            {
                break;
            }

            line = state.lines_queue.front();
            state.lines_queue.pop();
        }

        bool request_complete = parse_http_line(line, req, parse_state, expected_content_length);

        if (parse_state == ParseState::Error)
        {
            response resp;
            resp.statusCode = 400; // bad request
            resp.statusText = "Bad Request";
            resp.body = "<h1>400 - Malformed/Bad Request</h1>";

            sendResponse(clientsocket, resp);
            cout << RED << "✖ [ERROR] Malformed/Bad request detected. Response Sent [Status: 400]" << RESET << "\n";

            break;
        }

        if (request_complete)
        {
            print_request(req);
            response resp;

            if (!validateRequest(req))
            {
                std::cerr << RED << BOLD << "✖ [ERROR] Request is invalid" << RESET << "\n";
                resp.statusCode = 400;
                resp.statusText = "Bad Request";
                resp.body = "<h1>400 - Bad Request</h1>";
            }
            else
            {
                routing_key current_key = {req.method, req.uri};
                auto route_iterator = router.find(current_key);
                if (route_iterator != router.end())
                {
                    resp = route_iterator->second(req);
                }
                else if (req.method == "GET" && endsWith(req.uri, ".html") && (req.uri.find("..") == std::string::npos))
                {
                    resp = handle_html_file(req);
                }
                else
                {
                    resp = handle_404(req);
                }
            }

            sendResponse(clientsocket, resp);
            cout << GREEN << "✔ [SUCCESS] Response Sent [Status: " << resp.statusCode << "]" << RESET << "\n";

            parse_state = ParseState::RequestLine;
            req = request();
            expected_content_length = 0;
        }
    }

    if (producer.joinable())
    {
        producer.join();
    }
    close(clientsocket);
    cout << BLUE << "ℹ [INFO] Client disconnected. Socket closed." << RESET << "\n";
}

int main()
{
    int serverFd = get_socket_server();
    if (serverFd < 0)
        return 1;

    routing_table router;
    router[{"GET", "/"}] = handle_homepage;
    router[{"GET", "/api/data"}] = handle_api_data;

    std::cout << "\n"
              << GREEN << BOLD << "🚀 Server is running concurrently on port 42069..." << RESET << "\n\n";

    while (1)
    {
        int clientSocket = accept(serverFd, nullptr, nullptr);

        if (clientSocket < 0)
        {
            std::cerr << RED << BOLD << "✖ [ERROR] Error accepting client connection" << RESET << "\n";
            continue;
        }

        cout << BLUE << BOLD << "\n🔗 [INFO] New client connection accepted!" << RESET << "\n";

        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        std::thread worker(handle_client, clientSocket, std::cref(router));
        worker.detach();
    }

    close(serverFd);
}