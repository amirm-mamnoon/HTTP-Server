cmake_minimum_required(VERSION 3.10)
project(serv)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags
add_compile_options(-O3 -Wall -Wextra)

# Find pthread (required for std::thread, std::mutex)
find_package(Threads REQUIRED)

# Define the executable
add_executable(serv.out
    main.cpp
    src/requests/requests.cpp
    src/responses/responses.cpp
    src/validator/validator.cpp
    src/router/router.hpp
    src/handler/handler.cpp
)

# Link pthread
target_link_libraries(serv.out PRIVATE Threads::Threads)