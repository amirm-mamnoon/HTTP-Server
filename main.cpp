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

int main()
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        std::cerr << "Failed to open file!\n";
        return 1;
    }

    std::thread producer(pars_fd_to_queue, fd);

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
    cout << "\n";
    return 0;
}