#include "responses.hpp"

#include <iostream>
#include <sys/socket.h>
#include <sstream>

bool sendResponse(int fd, const response &resp)
{
    /*
    structure of response:
        httpVersion statusCode statusText\r\n
        headers: (Don't forget the content-length)
            key: value\r\n
        \r\n
        body\r\n

    */
    std::stringstream ss;

    ss << resp.httpVersion << " " << resp.statusCode << " " << resp.statusText << "\r\n";

    for (const auto &h : resp.headers)
        ss << h.key << ": " << h.value << "\r\n";

    ss << "Content-Length: " << resp.body.length() << "\r\n";

    ss << "\r\n";

    ss << resp.body << "\r\n";

    std::string fullResp = ss.str();

    size_t totalSent = 0;
    while (totalSent < fullResp.length())
    {
        ssize_t sent = send(fd, fullResp.c_str() + totalSent, fullResp.length() - totalSent, 0);

        if (sent < 0)
        {
            std::cerr << "Failed sending data\n";
            return false;
        }
        totalSent += sent;
    }
    return true;
}


bool createResponse(response& resp) { // make it more dynamic/request driven
    int flag = true;

    responseHeader h;
    h.key = "foo";
    h.value = "baz";
    resp.statusCode = 200;
    resp.statusText = "OK";
    resp.headers.push_back(h);
    resp.body = " { name: \"amir\", age: 24 } ";

    return flag;
}