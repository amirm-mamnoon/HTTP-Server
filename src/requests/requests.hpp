#pragma once

#include <iostream>
#include <vector>

using std::string;

struct header
{
    string key;
    string value;
};

struct request
{
    string method;
    string uri;
    string version;
    std::vector<header> header;
    string body;
};

void print_request(const request& req);
void trim(string& str);
bool pars_request_line(string line, request& req);
bool pars_header_line(string line, request& req); 