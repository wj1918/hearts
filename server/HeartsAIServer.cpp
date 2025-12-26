#include "HeartsAIServer.h"
#include "AIRequestHandler.h"
#include "JsonProtocol.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "../third_party/httplib.h"

#include <iostream>

namespace hearts {
namespace server {

HeartsAIServer::HeartsAIServer(const std::string& host, int port)
    : host_(host), port_(port), server_(new httplib::Server()) {
    setup_routes();
}

HeartsAIServer::~HeartsAIServer() {
    stop();
}

void HeartsAIServer::setup_routes() {
    // Health check endpoint
    server_->Get("/api/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(JsonProtocol::format_health(), "application/json");
    });

    // Get AI move endpoint
    server_->Post("/api/move", [](const httplib::Request& req, httplib::Response& res) {
        std::string response;
        try {
            AIRequestHandler handler;
            response = handler.handle_get_move(req.body);

            // Check if it's an error response
            try {
                json resp_json = json::parse(response);
                if (resp_json.value("status", "") == "error") {
                    res.status = 400;
                }
            } catch (...) {
                // If we can't parse our own response, something is very wrong
                res.status = 500;
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Unhandled exception in /api/move: " << e.what() << std::endl;
            response = JsonProtocol::format_error("INTERNAL_ERROR", std::string("Unhandled exception: ") + e.what());
            res.status = 500;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception in /api/move" << std::endl;
            response = JsonProtocol::format_error("INTERNAL_ERROR", "Unknown exception");
            res.status = 500;
        }

        res.set_content(response, "application/json");
    });

    // Play one move endpoint - simplified interface with default AI config
    server_->Post("/api/play-one", [](const httplib::Request& req, httplib::Response& res) {
        AIRequestHandler handler;
        std::string response = handler.handle_play_one_move(req.body);

        try {
            json resp_json = json::parse(response);
            if (resp_json.value("status", "") == "error") {
                res.status = 400;
            }
        } catch (...) {
            res.status = 500;
        }

        res.set_content(response, "application/json");
    });

    // CORS preflight handling
    server_->Options("/api/move", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    server_->Options("/api/play-one", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    // Add CORS headers to all responses (except OPTIONS which already has them)
    server_->set_post_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        if (req.method != "OPTIONS") {
            res.set_header("Access-Control-Allow-Origin", "*");
        }
    });

    // Error handler - only set content if it wasn't already set by the handler
    server_->set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        // If the handler already set a response body, don't overwrite it
        if (!res.body.empty()) {
            return;
        }

        std::string error_msg;
        switch (res.status) {
            case 404:
                error_msg = "Endpoint not found";
                break;
            case 405:
                error_msg = "Method not allowed";
                break;
            default:
                error_msg = "Server error";
        }
        res.set_content(JsonProtocol::format_error("HTTP_ERROR", error_msg), "application/json");
    });
}

void HeartsAIServer::run() {
    std::cout << "Hearts AI Server starting on " << host_ << ":" << port_ << std::endl;
    std::cout << "Endpoints:" << std::endl;
    std::cout << "  GET  /api/health   - Health check" << std::endl;
    std::cout << "  POST /api/move     - Compute AI move (full config)" << std::endl;
    std::cout << "  POST /api/play-one - Play one move (default config)" << std::endl;

    if (!server_->listen(host_.c_str(), port_)) {
        std::cerr << "Failed to start server on " << host_ << ":" << port_ << std::endl;
    }
}

void HeartsAIServer::stop() {
    if (server_) {
        server_->stop();
    }
}

} // namespace server
} // namespace hearts
