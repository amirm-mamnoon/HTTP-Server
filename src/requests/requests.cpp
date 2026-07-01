#include "requests.hpp"

#include "../validator/validator.hpp"

#include <iostream>

using std::string;
using std::cout;

const string SEPARATOR = "\r\n";

void print_request(const request& req) {
    cout << "=== HTTP Request ===\n";
    cout << "Method:  " << req.method << "\n";
    cout << "URI:     " << req.uri << "\n";
    cout << "Version: " << req.version << "\n";
    
    cout << "\nHeaders:\n";
    for (const auto& h : req.header) {
        cout << "  " << h.key << ": " << h.value << "\n";
    }
    
    cout << "\nBody:\n";
    cout << req.body << "\n";
    cout << "====================\n";
}


void trim(string &str)
{
    while (!str.empty() && (str.back() == '\r' || str.back() == '\n' || str.back() == ' '))
    {
        str.pop_back();
    }

    size_t start = str.find_first_not_of(" ");
    if (start != std::string::npos)
    {
        str = str.substr(start);
    }
}

bool pars_request_line(string line, request &req)
{
    trim(line);

    size_t space1 = line.find(' ');
    size_t space2 = line.find(' ', space1 + 1);

    if (space2 == std::string::npos || space2 == std::string::npos)
    {
        return false;
    }

    req.method = line.substr(0, space1);
    req.uri = line.substr(space1 + 1, space2 - space1 - 1);
    req.version = line.substr(space2 + 1);

    return true;
}

bool pars_header_line(string line, request &req)
{
    trim(line);

    size_t colon_pos = line.find(':');

    if (colon_pos == std::string::npos)
    {
        return false;
    }

    requestHeader h;
    h.key = line.substr(0, colon_pos);
    h.value = line.substr(colon_pos + 1);
    trim(h.value);

    req.header.push_back(h);

    return true;
}