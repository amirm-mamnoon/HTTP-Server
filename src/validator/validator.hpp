#pragma once

struct response;
struct request;

#include "../requests/requests.hpp"
#include "../responses/responses.hpp"

#include <optional>

enum class status_codes : int
{
    // SUCCESS
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,

    // CLIENT ERRORS
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    IM_A_TEAPOT = 418,
    TOO_MANY_REQUESTS = 429,

    // SERVER ERRORS
    INTERNAL_SERVER_ERROR = 500
};

enum class methods
{
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    HEAD,
    OPTIONS
};

enum class content_types
{
    APPLICATION_JSON,
    TEXT_PLAIN,
    TEXT_HTML,
    APPLICATION_OCTET_STREAM
};

bool validateRequest(request &req);
bool validateResponse(const response &res);

// helper functions
std::string methodToString(methods method);
std::optional<methods> stringToMethod(const std::string &str);
std::optional<content_types> stringToContentType(const std::string &str);
std::string contentTypeToString(content_types type);