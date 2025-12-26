#ifndef AI_REQUEST_HANDLER_H
#define AI_REQUEST_HANDLER_H

#include <string>
#include <memory>
#include "JsonProtocol.h"
#include "../Hearts.h"
#include "../iiMonteCarlo.h"
#include "../UCT.h"

namespace hearts {
namespace server {

class AIRequestHandler {
public:
    AIRequestHandler();
    ~AIRequestHandler();

    // Main entry point for handling move requests
    std::string handle_get_move(const std::string& json_request);

    // Simplified endpoint - play exactly one move with minimal config
    std::string handle_play_one_move(const std::string& json_request);

private:
    // Create AI player with given configuration
    Player* create_player(const AIConfig& config, HeartsGameState* game);

    // Compute AI move using the created player
    card compute_ai_move(HeartsGameState* game, Player* player);

    // Clean up algorithm resources (player is owned by game)
    void cleanup_player(Player* player);
};

} // namespace server
} // namespace hearts

#endif
