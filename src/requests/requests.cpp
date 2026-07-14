#include "requests.hpp"
#include "../validator/validator.hpp"
#include <iostream>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define DIM     "\033[2m"

using std::cout;
using std::string;

const string SEPARATOR = "\r\n";

void print_request(const request &req)
{
    cout << "\n" << CYAN << BOLD << "╭─────────── HTTP REQUEST ───────────╮" << RESET << "\n";
    
    cout << CYAN << "│ " << YELLOW << BOLD << "Method:  " << RESET << req.method << "\n";
    cout << CYAN << "│ " << YELLOW << BOLD << "URI:     " << RESET << req.uri << "\n";
    cout << CYAN << "│ " << YELLOW << BOLD << "Version: " << RESET << req.version << "\n";

    cout << CYAN << "├────────────── Headers ─────────────┤" << RESET << "\n";
    for (const auto &h : req.header)
    {
        cout << CYAN << "│ " << GREEN << h.key << ": " << RESET << DIM << h.value << RESET << "\n";
    }

    if (!req.body.empty() && req.body != "[LOG]: NO LOG")
    {
        cout << CYAN << "├──────────────── Body ──────────────┤" << RESET << "\n";
        cout << CYAN << "│ " << RESET << req.body << "\n";
    }
    
    cout << CYAN << BOLD << "╰────────────────────────────────────╯" << RESET << "\n";
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

    if (space2 == std::string::npos || space1 == std::string::npos)
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