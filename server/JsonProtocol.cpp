#include "JsonProtocol.h"
#include <chrono>

namespace hearts {
namespace server {

// Parse card string like "AS", "10H", "2C" to internal card representation
// Format: {rank}{suit} where suit is S/D/C/H
card JsonProtocol::json_to_card(const json& j) {
    std::string card_str = j.get<std::string>();

    // Extract suit (last character)
    char suit_char = card_str.back();
    int suit;
    switch (suit_char) {
        case 'S': suit = 0; break;  // Spades
        case 'D': suit = 1; break;  // Diamonds
        case 'C': suit = 2; break;  // Clubs
        case 'H': suit = 3; break;  // Hearts
        default: throw std::runtime_error("Invalid suit: " + std::string(1, suit_char));
    }

    // Extract rank (everything except last character)
    std::string rank_str = card_str.substr(0, card_str.length() - 1);
    int rank;
    if (rank_str == "A") rank = 0;
    else if (rank_str == "K") rank = 1;
    else if (rank_str == "Q") rank = 2;
    else if (rank_str == "J") rank = 3;
    else if (rank_str == "10") rank = 4;
    else if (rank_str == "9") rank = 5;
    else if (rank_str == "8") rank = 6;
    else if (rank_str == "7") rank = 7;
    else if (rank_str == "6") rank = 8;
    else if (rank_str == "5") rank = 9;
    else if (rank_str == "4") rank = 10;
    else if (rank_str == "3") rank = 11;
    else if (rank_str == "2") rank = 12;
    else throw std::runtime_error("Invalid rank: " + rank_str);

    return Deck::getcard(suit, rank);
}

// Convert internal card to string like "AS", "10H", "2C"
json JsonProtocol::card_to_json(card c) {
    static const char* suits = "SDCH";
    static const char* ranks[] = {"A", "K", "Q", "J", "10", "9", "8", "7", "6", "5", "4", "3", "2"};

    int suit = Deck::getsuit(c);
    int rank = Deck::getrank(c);

    return std::string(ranks[rank]) + suits[suit];
}

std::vector<card> JsonProtocol::json_to_hand(const json& j) {
    std::vector<card> hand;
    for (const auto& card_json : j) {
        hand.push_back(json_to_card(card_json));
    }
    return hand;
}

int JsonProtocol::parse_rules(const json& rules_json) {
    int rules = 0;

    if (rules_json.value("queen_penalty", true)) {
        rules |= kQueenPenalty;
    }
    if (rules_json.value("jack_bonus", false)) {
        rules |= kJackBonus;
    }
    if (rules_json.value("no_trick_bonus", false)) {
        rules |= kNoTrickBonus;
    }
    if (rules_json.value("must_break_hearts", true)) {
        rules |= kMustBreakHearts;
    }
    if (rules_json.value("queen_breaks_hearts", true)) {
        rules |= kQueenBreaksHearts;
    }
    if (rules_json.value("do_pass_cards", false)) {
        rules |= kDoPassCards;
    }
    if (rules_json.value("no_hearts_first_trick", true)) {
        rules |= kNoHeartsFirstTrick;
    }
    if (rules_json.value("no_queen_first_trick", true)) {
        rules |= kNoQueenFirstTrick;
    }
    if (rules_json.value("lead_clubs", true)) {
        rules |= kLeadClubs;
    }
    if (rules_json.value("lead_2_clubs", false)) {
        rules |= kLead2Clubs;
    }

    return rules;
}

GameStateData JsonProtocol::parse_game_state(const json& j) {
    GameStateData data;

    // Parse player hand (single hand for current player)
    if (j.contains("player_hand")) {
        data.player_hand = json_to_hand(j.at("player_hand"));
    }

    // Current player (always 0)
    data.current_player = j.value("current_player", 0);

    // Current trick
    if (j.contains("current_trick")) {
        const auto& trick = j.at("current_trick");
        data.trick_lead_player = trick.value("lead_player", 0);
        if (trick.contains("cards")) {
            for (const auto& tc : trick.at("cards")) {
                TrickCard tcard;
                tcard.player = tc.at("player").get<int>();
                tcard.c = json_to_card(tc.at("card"));
                data.current_trick_cards.push_back(tcard);
            }
        }
    } else {
        data.trick_lead_player = data.current_player;
    }

    // Trick history (all completed tricks)
    if (j.contains("trick_history")) {
        for (const auto& trick_json : j.at("trick_history")) {
            CompletedTrick trick;
            trick.lead_player = trick_json.value("lead_player", 0);
            trick.winner = trick_json.value("winner", 0);
            if (trick_json.contains("cards")) {
                for (const auto& tc : trick_json.at("cards")) {
                    TrickCard tcard;
                    tcard.player = tc.at("player").get<int>();
                    tcard.c = json_to_card(tc.at("card"));
                    trick.cards.push_back(tcard);
                }
            }
            data.trick_history.push_back(trick);
        }
    }

    // Played cards (cards already won in previous tricks)
    data.played_cards.resize(4);
    if (j.contains("played_cards")) {
        const auto& played = j.at("played_cards");
        for (size_t i = 0; i < 4 && i < played.size(); i++) {
            data.played_cards[i] = json_to_hand(played[i]);
        }
    }

    // Scores
    data.scores.resize(4, 0.0);
    if (j.contains("scores")) {
        const auto& scores = j.at("scores");
        for (size_t i = 0; i < 4 && i < scores.size(); i++) {
            data.scores[i] = scores[i].get<double>();
        }
    }

    // Hearts broken
    data.hearts_broken = j.value("hearts_broken", false);

    // Pass direction
    data.pass_direction = j.value("pass_direction", 0);

    // Rules - can be either an integer bitmask or an object with individual flags
    if (j.contains("rules")) {
        const auto& rules_val = j.at("rules");
        if (rules_val.is_number()) {
            // Integer bitmask
            data.rules = rules_val.get<int>();
        } else if (rules_val.is_object()) {
            // Object with individual flags
            data.rules = parse_rules(rules_val);
        } else {
            // Default rules
            data.rules = kQueenPenalty | kMustBreakHearts | kQueenBreaksHearts |
                         kNoHeartsFirstTrick | kNoQueenFirstTrick | kLeadClubs;
        }
    } else {
        // Default rules
        data.rules = kQueenPenalty | kMustBreakHearts | kQueenBreaksHearts |
                     kNoHeartsFirstTrick | kNoQueenFirstTrick | kLeadClubs;
    }

    return data;
}

AIConfig JsonProtocol::parse_ai_config(const json& j) {
    AIConfig config;

    if (j.contains("ai_config")) {
        const auto& ai = j.at("ai_config");
        config.simulations = ai.value("simulations", 10000);
        config.worlds = ai.value("worlds", 30);
        config.epsilon = ai.value("epsilon", 0.1);
        config.use_threads = ai.value("use_threads", true);
        config.player_type = ai.value("player_type", "safe_simple");
    }

    return config;
}

std::string JsonProtocol::format_move_response(card c, int player, double time_ms) {
    json response = {
        {"status", "success"},
        {"move", {
            {"card", card_to_json(c)},
            {"player", player}
        }},
        {"computation_time_ms", time_ms}
    };
    return response.dump();
}

std::string JsonProtocol::format_error(const std::string& error_code, const std::string& message) {
    json response = {
        {"status", "error"},
        {"error_code", error_code},
        {"message", message}
    };
    return response.dump();
}

std::string JsonProtocol::format_health() {
    json response = {
        {"status", "ok"}
    };
    return response.dump();
}

} // namespace server
} // namespace hearts
