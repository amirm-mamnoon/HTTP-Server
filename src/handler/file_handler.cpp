#include "file_handler.hpp"

#include <fstream>
#include <sstream>

const std::string suffix__ = ".html";

bool endsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() < suffix.length())
    {
        return false;
    }

    return str.rfind(suffix) == (str.length() - suffix.length());
}

std::optional<std::string> read_from_file(request &req)
{

    std::string uri = req.uri;
    if (!endsWith(uri, suffix__))
    {
        return std::nullopt;
    }

    std::string filename;

    filename = "public" + uri;

    std::ifstream file(filename);

    if (!file.is_open())
    {
        return std::nullopt;
    }

    std::stringstream buffer;

    buffer << file.rdbuf();

    return buffer.str();
}