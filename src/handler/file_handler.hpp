#pragma once

#include <string>
#include <optional>

#include "../requests/requests.hpp"

std::optional<std::string> read_from_file(request &req);

/* helper function(s) */
bool endsWith(const std::string &str, const std::string &suffix);