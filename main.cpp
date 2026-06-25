#include <cstring>
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;
using std::string;

const char *filename = "messages.txt";
const int chunk_size = 8;
const int sentence_max_size = 1024;

void print_buffer(const char *buffer, int length)
{
    cout << "read: ";
    for (int i = 0; i < length; i++)
    {
        cout << buffer[i];
    }
}

int main()
{

    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "cannot open the file\n";
        return 1;
    }

    char buffer[chunk_size];
    char sentence_buffer[sentence_max_size];
    int sentence_pos = 0;
    while (file.read(buffer, chunk_size) || file.gcount() > 0)
    {

        int bytesRead = file.gcount();
        for (int i = 0; i < bytesRead; i++)
        {
            if (sentence_pos < sentence_max_size - 1)
            {
                sentence_buffer[sentence_pos++] = buffer[i];

                if (buffer[i] == '\n')
                {
                    print_buffer(sentence_buffer, sentence_pos);
                    memset(sentence_buffer, 0, sizeof(sentence_buffer));
                    sentence_pos = 0;
                }
            }
        }
        memset(buffer, 0, sizeof(buffer));
    }

    if (sentence_pos > 0)
    {
        print_buffer(sentence_buffer, sentence_pos);
    }
        cout << endl;

    file.close();
    return 0;
}