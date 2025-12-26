#ifndef HEARTS_AI_SERVER_H
#define HEARTS_AI_SERVER_H

#include <string>
#include <memory>

// Forward declare httplib types to avoid including heavy header here
namespace httplib {
    class Server;
}

namespace hearts {
namespace server {

class HeartsAIServer {
public:
    HeartsAIServer(const std::string& host = "0.0.0.0", int port = 8080);
    ~HeartsAIServer();

    // Start the server (blocking)
    void run();

    // Stop the server
    void stop();

    // Get server info
    std::string get_host() const { return host_; }
    int get_port() const { return port_; }

private:
    void setup_routes();

    std::string host_;
    int port_;
    std::unique_ptr<httplib::Server> server_;
};

} // namespace server
} // namespace hearts

#endif
