#ifndef JSON_PROTOCOL_H
#define JSON_PROTOCOL_H

#include <string>
#include <vector>
#include "../CardGameState.h"
#include "../Hearts.h"
#include "../third_party/json.hpp"

namespace hearts {
namespace server {

using json = nlohmann::json;

struct AIConfig {
    int simulations = 10000;
    int worlds = 30;
    double epsilon = 0.1;
    bool use_threads = true;
    std::string player_type = "safe_simple";
};

struct TrickCard {
    int player;
    card c;
};

struct CompletedTrick {
    std::vector<TrickCard> cards;  // 4 cards played in order
    int lead_player;
    int winner;
};

struct GameStateData {
    std::vector<card> player_hand;  // Single hand for current player
    int current_player;             // Always 0
    std::vector<TrickCard> current_trick_cards;
    int trick_lead_player;
    std::vector<CompletedTrick> trick_history;  // All completed tricks
    std::vector<std::vector<card>> played_cards;
    std::vector<double> scores;
    bool hearts_broken;
    int pass_direction;
    int rules;
};

class JsonProtocol {
public:
    // Parsing
    static GameStateData parse_game_state(const json& j);
    static AIConfig parse_ai_config(const json& j);
    static int parse_rules(const json& rules_json);

    // Formatting responses
    static std::string format_move_response(card c, int player, double time_ms);
    static std::string format_error(const std::string& error_code, const std::string& message);
    static std::string format_health();

    // Card conversion
    static card json_to_card(const json& j);
    static json card_to_json(card c);

private:
    static std::vector<card> json_to_hand(const json& j);
};

} // namespace server
} // namespace hearts

#endif
