#pragma once

#include "../responses/responses.hpp"
#include "../requests/requests.hpp"

response handle_homepage(request req);
response handle_api_data(request req);
response handle_404(request req);
response handle_html_file(request req);