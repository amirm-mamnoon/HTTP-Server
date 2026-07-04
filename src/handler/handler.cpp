#include "handler.hpp"

#include "file_handler.hpp"

std::string file_missing_err_msg = "<h1>500 - Server Configuration Error</h1><p>Missing index.html</p>";

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

response handle_html_file(request req)
{
    response resp;

    auto file_content = read_from_file(req);

    if (file_content.has_value())
    {
        resp.statusCode = 200;
        resp.statusText = "OK";
        resp.body = file_content.value();
        resp.headers.push_back({"Content-Type", "text/html"});
    }
    else
    {
        resp.statusCode = 500;
        resp.statusText = "Internal Server Error";
        resp.body = file_missing_err_msg;
        resp.headers.push_back({"Content-Type", "text/html"});
    }

    return resp;
}