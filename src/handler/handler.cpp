#include "handler.hpp"

response handle_homepage(request /* req */)
{
    response resp;
    resp.statusCode = 200;
    resp.statusText = "OK";
    resp.body = "<h1>Welcome to the Homepage!</h1>";
    resp.headers.push_back({"Content-Type", "text/html"});
    return resp;
}

response handle_api_data(request /* req */)
{
    response resp;
    resp.statusCode = 200;
    resp.statusText = "OK";
    resp.body = "{ \"name\": \"amir\", \"age\": 24 }";
    resp.headers.push_back({"Content-Type", "application/json"});
    return resp;
}

response handle_404(request /* req */)
{
    response resp;
    resp.statusCode = 404;
    resp.statusText = "Not Found";
    resp.body = "<h1>404 - Page Not Found</h1>";
    resp.headers.push_back({"Content-Type", "text/html"});
    return resp;
}