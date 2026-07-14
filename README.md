# HTTP-Server

A HTTP/1.1 server built **from scratch in C++**, using raw POSIX sockets — no third-party HTTP libraries (no Boost.Asio, no libmicrohttpd, no cpp-httplib). It handles TCP connections directly, parses raw request bytes off the wire, and writes the HTTP response back byte-for-byte.

The goal of this project is to understand what actually happens under the hood of a web server: socket lifecycle, request parsing state machines, concurrency with threads, and routing — all without a framework doing the work for you.

## Features

- **Raw socket server** — binds, listens, and accepts connections using `socket()`, `bind()`, `listen()`, `accept()` directly (`sys/socket.h`, `netinet/in.h`).
- **Concurrent per-client handling** — every accepted connection is served on its own detached `std::thread`, so multiple clients are handled at once.
- **Producer/consumer request parsing** — within each client thread, a dedicated producer thread reads the socket in small (8-byte) chunks and pushes complete lines onto a thread-safe queue; the consumer (main handler loop) pulls lines off the queue and feeds them into a request parser. Synchronization is done with `std::mutex` and `std::condition_variable`.
- **Streaming/stateful HTTP parser** — a small state machine (`ParseState::RequestLine → Headers → Body`) parses the request line, headers, and body incrementally as lines arrive, rather than requiring the full request up front.
- **Request validation** — validates HTTP method, checks `Content-Length` bounds (rejects negative or over 10 MB), and validates `Content-Type` against a known set.
- **Static file serving** — serves `.html` files out of a local `public/` directory, with basic path-traversal protection (rejects URIs containing `..`).
- **Simple router** — a `std::map`-based routing table keyed on `(method, uri)` pairs, dispatching to handler functions.
- **Built-in routes**:
  - `GET /` → homepage
  - `GET /api/data` → sample JSON response
  - `GET /*.html` → serves matching file from `public/`
  - anything unmatched → `404 Not Found`
- **Timeouts** — per-socket receive/send timeouts (`SO_RCVTIMEO` / `SO_SNDTIMEO`) so slow or dead clients don't hang a thread forever.
- **Colorized console logging** — pretty-printed request/response logs with ANSI colors for quick visual debugging.

## Project structure

```
HTTP-Server/
├── main.cpp                     # Entry point: socket setup, accept loop, thread spawning, request/response wiring
├── CMakeLists.txt                # Build configuration
├── messages.txt                  # Scratch/test file used during early chunked-read experiments
└── src/
    ├── requests/
    │   ├── requests.hpp/.cpp      # `request` struct, request-line & header-line parsing, pretty-printer
    ├── responses/
    │   ├── responses.hpp/.cpp     # `response` struct, raw response serialization + socket send
    ├── router/
    │   └── router.hpp             # `routing_key` + `routing_table` (method + URI → handler function map)
    ├── handler/
    │   ├── handler.hpp/.cpp        # Route handlers (homepage, /api/data, 404, static HTML)
    │   └── file_handler.hpp/.cpp   # Reads `.html` files from the `public/` directory
    └── validator/
        └── validator.hpp/.cpp      # Method/Content-Type enums + string conversion, request validation
```

## How it works

1. **`main()`** opens a listening socket on port `42069` and enters an infinite `accept()` loop.
2. Each accepted client is handed off to `handle_client()` on its own detached thread, along with a `const` reference to the shared routing table.
3. Inside `handle_client()`, a **producer thread** (`pars_fd_to_queue`) reads the raw socket in 8-byte chunks and reassembles them into `\n`-terminated lines, pushing each complete line onto a shared queue guarded by a mutex + condition variable.
4. The **consumer** (the client-handling thread itself) waits on the condition variable, pops lines as they arrive, and feeds them through `parse_http_line()`, a state machine that builds up the request line, then headers, then body (using `Content-Length` to know when the body is complete).
5. Once a full request is parsed, it's validated (`validateRequest`), looked up in the routing table, and dispatched to the matching handler — or served as a static `.html` file, or answered with a `404`.
6. The handler returns a `response` struct, which `sendResponse()` serializes into a raw HTTP/1.1 response (status line, headers, `Content-Length`, body) and writes back to the socket.
7. The producer thread is joined and the socket is closed once the client disconnects.

## Build

Requirements:
- CMake ≥ 3.10
- `clang++` with C++17 support (set explicitly in `CMakeLists.txt`; swap to `g++` if you prefer)
- POSIX threads (`pthread`, linked automatically via `find_package(Threads)`)
- A Linux/Unix environment (uses `sys/socket.h`, `netinet/in.h`, `unistd.h`, etc. — this will not build on Windows without WSL or a POSIX layer)

```bash
git clone https://github.com/amirm-mamnoon/HTTP-Server.git
cd HTTP-Server
mkdir build && cd build
cmake ..
make
```

This produces an executable named `serv.out`.

## Run

```bash
./serv.out
```

The server starts listening on **port 42069**:

```
🚀 Server is running concurrently on port 42069...
```

## Try it out

```bash
# Homepage
curl http://localhost:42069/

# Sample JSON endpoint
curl http://localhost:42069/api/data

# 404 handling
curl -i http://localhost:42069/does-not-exist

# Static HTML (create public/about.html first)
curl http://localhost:42069/about.html
```

To serve static pages, create a `public/` directory next to the executable and drop `.html` files in it — e.g. `public/about.html` will be served at `GET /about.html`.

## Skills demonstrated

This project was built to get hands-on with systems-level concepts that frameworks usually hide:

- **Network/socket programming** — direct use of the POSIX sockets API (`socket`, `bind`, `listen`, `accept`, `read`, `send`).
- **Concurrency** — multithreading with `std::thread`, synchronized via `std::mutex` and `std::condition_variable` in a producer/consumer pattern.
- **Protocol implementation** — hand-rolled HTTP/1.1 request parsing (request line, headers, body) as an incremental state machine, plus manual response serialization.
- **Software design** — a decoupled architecture separating request parsing, response building, validation, routing, and handlers into distinct modules.
- **Build tooling** — CMake-based build configuration with compiler warnings enabled (`-Wall -Wextra`) and optimization flags.

## Design notes / known limitations

- **One producer thread per connection** — this is simple and demonstrates the producer/consumer pattern clearly, but it means every connection costs at least two threads. A thread pool would scale far better under load; this is a known area for improvement.
- **No HTTP keep-alive** — each connection is treated as a single request/response cycle; there's no persistent-connection reuse yet.
- **No HTTPS/TLS support.**
- **Minimal routing** — routes are hardcoded in `main()`; there's no support for path parameters or wildcards beyond the `.html` static-file fallback.

## Possible next steps

- Replace the per-connection thread model with a fixed-size thread pool + task queue.
- Add HTTP keep-alive / persistent connections.
- Support additional static file types (CSS, JS, images) instead of just `.html`.
- Add basic unit tests around the parser and validator.
- Add graceful shutdown handling (currently the accept loop runs forever with no signal handling, despite `<csignal>` being included).

## License

This project is licensed under the MIT License.
