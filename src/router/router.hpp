#pragma once

#include "../requests/requests.hpp"
#include "../responses/responses.hpp"

#include <functional>
#include <map>
#include <string>

struct routing_key
{
    std::string method;
    std::string uri;

    bool operator<(const routing_key &other) const
    {
        if (method != other.method)
        {
            return method < other.method;
        }

        return uri < other.uri;
    }
};

typedef std::map<routing_key, std::function<response(request)>> routing_table;