#pragma once

#include <iostream>
#include <vector>

#include "../validator/validator.hpp"

struct requestHeader
{
    std::string key;
    std::string value;
};

struct request
{
    std::string method;
    std::string uri;
    std::string version;
    std::vector<requestHeader> header;
    std::string body;
};

void print_request(const request &req);
void trim(std::string &str);
bool pars_request_line(std::string line, request &req);
bool pars_header_line(std::string line, request &req);