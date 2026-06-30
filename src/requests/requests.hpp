#pragma once

#include <iostream>
#include <vector>

#include "../validator/validator.hpp"

using std::string;

struct requestHeader 
{
    string key;
    string value;
};

struct request
{
    methods method;
    string uri;
    string version;
    std::vector<requestHeader> header;
    string body;
};

void print_request(const request& req);
void trim(string& str);
bool pars_request_line(string line, request& req);
bool pars_header_line(string line, request& req); 