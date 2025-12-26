#include "HeartsAIServer.h"
#include <iostream>
#include <cstdlib>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace hearts::server;

static HeartsAIServer* g_server = nullptr;

#ifdef _WIN32
BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT) {
        std::cout << "\nShutting down server..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
        return TRUE;
    }
    return FALSE;
}
#else
void signal_handler(int signal) {
    std::cout << "\nShutting down server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}
#endif

void print_usage(const char* program_name) {
    std::cout << "Hearts AI Server" << std::endl;
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [port] [host]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  port  - Port number to listen on (default: 8080)" << std::endl;
    std::cout << "  host  - Host address to bind to (default: 0.0.0.0)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << "              # Listen on 0.0.0.0:8080" << std::endl;
    std::cout << "  " << program_name << " 3000         # Listen on 0.0.0.0:3000" << std::endl;
    std::cout << "  " << program_name << " 8080 127.0.0.1 # Listen on localhost:8080" << std::endl;
    std::cout << std::endl;
    std::cout << "API Endpoints:" << std::endl;
    std::cout << "  GET  /api/health  - Health check, returns {\"status\": \"ok\"}" << std::endl;
    std::cout << "  POST /api/move    - Compute AI move for given game state" << std::endl;
    std::cout << std::endl;
    std::cout << "Example request to /api/move:" << std::endl;
    std::cout << R"(  curl -X POST http://localhost:8080/api/move \)" << std::endl;
    std::cout << R"(    -H "Content-Type: application/json" \)" << std::endl;
    std::cout << R"(    -d '{"game_state": {"player_hands": [[...]], ...}}')" << std::endl;
}

int main(int argc, char* argv[]) {
    // Check for help flag
    if (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        print_usage(argv[0]);
        return 0;
    }

    // Parse arguments
    int port = 8080;
    std::string host = "0.0.0.0";

    if (argc >= 2) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
            return 1;
        }
    }

    if (argc >= 3) {
        host = argv[2];
    }

    // Set up signal handler for graceful shutdown
#ifdef _WIN32
    SetConsoleCtrlHandler(console_handler, TRUE);
#else
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
#endif

    try {
        HeartsAIServer server(host, port);
        g_server = &server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
