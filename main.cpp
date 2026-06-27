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

using std::cout;

const char *filename = "messages.txt";
const int chunk_size = 8;

std::queue<std::string> lines_queue;
std::mutex queue_mutex;
std::condition_variable cv;
bool parsing_finished = 0;

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

int get_from_file(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        std::cerr << "Failed to open file!\n";
        return -1;
    }

    return fd;
}

int get_socket_server() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(42069);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "error bind\n"; close(fd); return -1;
    }
    if (listen(fd, 5) < 0) {
        std::cerr << "error listen\n"; close(fd); return -1;
    }

    return fd;
}

int get_from_socket(int fd) {
    int clientSocket = accept(fd, nullptr, nullptr);
    if (clientSocket < 0) {
        close(clientSocket);
        return -1;
    }
    return clientSocket;
}

int main()
{
    int fd = get_socket_server();
    if (fd < 0)
    {
        return 1;
    }

    int clientSocket = get_from_socket(fd);
    if (clientSocket < 0) {
        std::cerr << "error client\n"; close(fd); return 1;
    }

    std::thread producer(pars_fd_to_queue, clientSocket);

    std::string line;
    while (1)
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        cv.wait(lock, []
                { return !lines_queue.empty() || parsing_finished; });

        while (!lines_queue.empty())
        {
            line = lines_queue.front();
            lines_queue.pop();

            cout << "read: " << line;
        }

        if (parsing_finished && lines_queue.empty())
        {
            break;
        }
    }

    if (producer.joinable())
    {
        producer.join();
    }
    close(fd);
    close(clientSocket);
    cout << "\n";
    return 0;
}