#pragma once

#include <string>
#include <vector>

struct responseHeader
{
    std::string key;
    std::string value;
};

struct response
{
    std::string httpVersion;
    unsigned statusCode;
    std::string statusText;
    std::vector<responseHeader> headers;
    std::string body;

    response() : httpVersion("HTTP/1.1"), statusCode(200), statusText("OK") {}
};

bool sendResponse(int fd, const response &resp);
bool createResponse(response &resp, bool isOkRequest);