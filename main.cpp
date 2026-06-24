#include <iostream>
#include <fstream>

const char *filename = "messages.txt";
const int chunk_size = 8;

int main()
{

    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "cannot open the file\n";
        return 1;
    }

    char buffer[chunk_size];
    while (file.read(buffer, chunk_size) || file.gcount() > 0)
    {

        int bytesRead = file.gcount();
        std::cout << "read: ";
        for (int i = 0; i < bytesRead; i++)
        {
            std::cout << buffer[i];
        }
        std::cout << std::endl;
    }

    file.close();
    return 0;
}