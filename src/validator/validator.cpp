#include "validator.hpp"

std::string methodToString(methods method)
{
    switch (method)
    {
    case methods::DELETE:
        return "DELETE";
    case methods::GET:
        return "GET";
    case methods::HEAD:
        return "HEAD";
    case methods::OPTIONS:
        return "OPTIONS";
    case methods::PATCH:
        return "PATCH";
    case methods::POST:
        return "POST";
    case methods::PUT:
        return "PUT";
    }
    return "UNKNOWN";
}
std::optional<methods> stringToMethod(const std::string &str)
{
    if (str == "GET")
        return methods::GET;
    if (str == "POST")
        return methods::POST;
    if (str == "DELETE")
        return methods::DELETE;
    if (str == "PATCH")
        return methods::PATCH;
    if (str == "PUT")
        return methods::PUT;
    if (str == "HEAD")
        return methods::HEAD;
    if (str == "OPTIONS")
        return methods::OPTIONS;

    return std::nullopt;
}

std::string contentTypeToString(content_types type)
{
    switch (type)
    {
    case content_types::APPLICATION_JSON:
        return "application/json";
    case content_types::TEXT_PLAIN:
        return "text/plain";
    case content_types::TEXT_HTML:
        return "text/html";
    case content_types::APPLICATION_OCTET_STREAM:
        return "application/octet-stream";
    }
    return "UNKNOWN";
}

std::optional<content_types> stringToContentType(const std::string &str)
{
    if (str == "application/json")
        return content_types::APPLICATION_JSON;
    if (str == "text/plain")
        return content_types::TEXT_PLAIN;
    if (str == "text/html")
        return content_types::TEXT_HTML;
    if (str == "application/octet-stream")
        return content_types::APPLICATION_OCTET_STREAM;

    return std::nullopt;
}

bool validateMethod__(const request &req)
{
    std::string method = req.method;
    for (auto &c : method)
        c = std::tolower(static_cast<unsigned char>(c));
    if (method == "get")
        return true;
    if (method == "post")
        return true;
    if (method == "put")
        return true;
    if (method == "patch")
        return true;
    if (method == "delete")
        return true;
    if (method == "options")
        return true;
    if (method == "head")
        return true;

    return false;
}

int validateContentType__(const request &req)
{
    for (const auto &h : req.header)
    {
        if (h.key == "Content-Type")
        {
            auto ct = stringToContentType(h.value);
            if (!ct)
            {
                return 0;
            }
            return 1;
        }
    }
    return -1;
}

bool validateRequest(request &req)
{
    if (!validateMethod__(req))
        return false;

    if (validateContentType__(req) == 0)
        return false;

    return true;
}