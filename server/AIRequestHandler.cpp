#include "AIRequestHandler.h"
#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace hearts {
namespace server {

// Helper to convert card to readable string
static std::string card_to_string(card c) {
    static const char* suits[] = {"S", "D", "C", "H"};
    static const char* ranks[] = {"A", "K", "Q", "J", "10", "9", "8", "7", "6", "5", "4", "3", "2"};
    // Card encoding: card = (suit << 4) + rank
    int suit = c >> 4;
    int rank = c & 0xF;
    return std::string(ranks[rank]) + suits[suit];
}

AIRequestHandler::AIRequestHandler() {
}

AIRequestHandler::~AIRequestHandler() {
}

std::string AIRequestHandler::handle_get_move(const std::string& json_request) {
    HeartsGameState* game = nullptr;
    Player* player = nullptr;

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::cout << "\n========== /api/move REQUEST ==========" << std::endl;
        std::cout << "[DEBUG] Raw JSON request:\n" << json_request << std::endl;

        // Parse JSON request
        json request_json = json::parse(json_request);

        // Parse game state and AI config
        GameStateData state_data = JsonProtocol::parse_game_state(request_json.at("game_state"));
        AIConfig config = JsonProtocol::parse_ai_config(request_json);

        // Debug: Print game state
        std::cout << "[DEBUG] Current player: " << state_data.current_player << std::endl;
        std::cout << "[DEBUG] Hearts broken: " << (state_data.hearts_broken ? "yes" : "no") << std::endl;
        std::cout << "[DEBUG] Player hand: ";
        for (size_t i = 0; i < state_data.player_hand.size(); i++) {
            if (i > 0) std::cout << " ";
            std::cout << card_to_string(state_data.player_hand[i]);
        }
        std::cout << " (" << state_data.player_hand.size() << " cards)" << std::endl;
        if (!state_data.current_trick_cards.empty()) {
            std::cout << "[DEBUG] Current trick: ";
            for (const auto& tc : state_data.current_trick_cards) {
                std::cout << "P" << tc.player << ":" << card_to_string(tc.c) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "[DEBUG] AI config: sims=" << config.simulations
                  << ", epsilon=" << config.epsilon
                  << ", threads=" << (config.use_threads ? "yes" : "no")
                  << ", type=" << config.player_type << std::endl;

        // Create game state and player together (player must be registered in game)
        game = new HeartsGameState(static_cast<int>(time(nullptr)));

        // Create the AI player first (it needs to be in the game's player list)
        player = create_player(config, nullptr);
        if (!player) {
            delete game;
            return JsonProtocol::format_error("AI_CONFIG_ERROR", "Failed to create AI player");
        }

        // Add players to game: AI player is always player 0
        for (int i = 0; i < 4; i++) {
            if (i == 0) {
                game->addPlayer(player);
            } else {
                game->addPlayer(new Player(0));
            }
        }

        // Now set the game state on the player
        player->setGameState(game);

        // Reset allocates tricks and deals random cards
        game->Reset();

        // Set up the game state from input data
        // Only current player's hand is provided (always player 0)
        for (int p = 0; p < 4; p++) {
            game->cards[p].reset();
            game->original[p].reset();
        }
        // Set current player's hand (player 0)
        for (card c : state_data.player_hand) {
            game->cards[0].set(c);
            game->original[0].set(c);
        }

        game->setPassDir(state_data.pass_direction);
        // Set first player based on trick lead if there's a current trick,
        // otherwise default to player 0
        int first_player = state_data.current_trick_cards.empty() ? 0 : state_data.trick_lead_player;
        game->setFirstPlayer(first_player);
        game->setRules(state_data.rules);

        // setFirstPlayer with kLead2Clubs may override currPlr to 0 when passDir != kHold
        // We need to reset currPlr to the trick lead player before applying current trick moves
        if (!state_data.current_trick_cards.empty()) {
            game->currPlr = state_data.trick_lead_player;
        }

        for (int p = 0; p < 4; p++) {
            for (card c : state_data.played_cards[p]) {
                game->taken[p].set(c);
                game->allplayed.set(c);
            }
        }

        // Replay trick history: add cards to hands and apply moves
        for (const CompletedTrick& trick : state_data.trick_history) {
            // Set currPlr to the lead player for this trick
            game->currPlr = trick.lead_player;
            // First, add all cards in this trick to respective players' hands
            for (const TrickCard& tc : trick.cards) {
                game->cards[tc.player].set(tc.c);
                game->original[tc.player].set(tc.c);
            }
            // Then apply all moves in order
            for (const TrickCard& tc : trick.cards) {
                CardMove* move = new CardMove(tc.c, tc.player);
                game->ApplyMove(move);
                delete move;
            }
        }

        // For current trick cards, we need to add them to respective players' hands
        // before applying moves (ApplyMove checks if player has the card)
        for (const TrickCard& tc : state_data.current_trick_cards) {
            game->cards[tc.player].set(tc.c);
            game->original[tc.player].set(tc.c);
        }

        // Now apply the current trick moves
        for (const TrickCard& tc : state_data.current_trick_cards) {
            CardMove* move = new CardMove(tc.c, tc.player);
            game->ApplyMove(move);
            delete move;
        }

        // Validate: check if there are legal moves
        Move* legal_moves = game->getMoves();
        if (!legal_moves) {
            std::cout << "[DEBUG] ERROR: No legal moves!" << std::endl;
            cleanup_player(player);
            delete game;
            return JsonProtocol::format_error("NO_LEGAL_MOVES", "No legal moves available in this game state");
        }

        // Count legal moves and print them
        int num_moves = 0;
        std::cout << "[DEBUG] Legal moves: ";
        for (Move* m = legal_moves; m; m = m->next) {
            CardMove* cm = dynamic_cast<CardMove*>(m);
            if (cm) {
                if (num_moves > 0) std::cout << ", ";
                std::cout << card_to_string(cm->c);
            }
            num_moves++;
        }
        std::cout << " (" << num_moves << " total)" << std::endl;

        card move;
        if (num_moves == 1) {
            // Only one legal move, no need to run AI
            CardMove* card_move = dynamic_cast<CardMove*>(legal_moves);
            move = card_move->c;
            std::cout << "[DEBUG] Single legal move, skipping AI" << std::endl;
        } else {
            // Multiple moves - run AI to choose best one
            std::cout << "[DEBUG] Running AI..." << std::endl;
            move = compute_ai_move(game, player);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        std::cout << "[DEBUG] Chosen move: " << card_to_string(move) << std::endl;
        std::cout << "[DEBUG] Computation time: " << time_ms << " ms" << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Format response (always player 0)
        std::string response = JsonProtocol::format_move_response(move, 0, time_ms);

        // Cleanup
        if (player) cleanup_player(player);
        delete game;

        return response;

    } catch (const json::exception& e) {
        std::cout << "[ERROR] JSON parse error: " << e.what() << std::endl;
        if (player) cleanup_player(player);
        if (game) delete game;
        return JsonProtocol::format_error("PARSE_ERROR", std::string("JSON parse error: ") + e.what());
    } catch (const std::exception& e) {
        std::cout << "[ERROR] Internal error: " << e.what() << std::endl;
        if (player) cleanup_player(player);
        if (game) delete game;
        return JsonProtocol::format_error("INTERNAL_ERROR", std::string("Internal error: ") + e.what());
    } catch (...) {
        std::cout << "[ERROR] Unknown error occurred" << std::endl;
        if (player) cleanup_player(player);
        if (game) delete game;
        return JsonProtocol::format_error("UNKNOWN_ERROR", "An unknown error occurred");
    }
}

Player* AIRequestHandler::create_player(const AIConfig& config, HeartsGameState* game) {
    double C = 0.4;  // UCT exploration constant
    int worlds = 30;  // Number of world models for iiMonteCarlo
    int sims_per_world = std::max(1, config.simulations / worlds);

    // Create UCT search with playout module
    UCT* uct = new UCT(sims_per_world, C);
    uct->setPlayoutModule(new HeartsPlayout());
    uct->setEpsilonPlayout(config.epsilon);

    // Wrap UCT in iiMonteCarlo for proper game state handling
    iiMonteCarlo* iimc = new iiMonteCarlo(uct, worlds);
    if (config.use_threads) {
        iimc->setUseThreads(true);
    }

    // Create player based on type
    SimpleHeartsPlayer* player;
    if (config.player_type == "safe_simple") {
        player = new SafeSimpleHeartsPlayer(iimc);
    } else if (config.player_type == "global") {
        player = new GlobalHeartsPlayer(iimc);
    } else if (config.player_type == "global2") {
        player = new GlobalHeartsPlayer2(iimc);
    } else if (config.player_type == "global3") {
        player = new GlobalHeartsPlayer3(iimc);
    } else {
        player = new SimpleHeartsPlayer(iimc);
    }

    // Set model level for opponent modeling
    player->setModelLevel(2);

    // Assign game state to player if provided
    if (game) {
        player->setGameState(game);
    }

    return player;
}

card AIRequestHandler::compute_ai_move(HeartsGameState* game, Player* player) {
    // Get move from AI
    Move* move = player->Play();

    if (!move) {
        // Fallback to first legal move if AI returns null
        Move* legal_moves = game->getMoves();
        if (legal_moves) {
            move = legal_moves;
        } else {
            throw std::runtime_error("No legal moves available");
        }
    }

    // Extract card from move
    CardMove* card_move = dynamic_cast<CardMove*>(move);
    if (!card_move) {
        throw std::runtime_error("Invalid move type returned by AI");
    }

    return card_move->c;
}

std::string AIRequestHandler::handle_play_one_move(const std::string& json_request) {
    HeartsGameState* game = nullptr;
    Player* player = nullptr;

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::cout << "\n========== /api/play-one REQUEST ==========" << std::endl;
        std::cout << "[DEBUG] Raw JSON request:\n" << json_request << std::endl;

        // Parse JSON request
        json request_json = json::parse(json_request);

        // Parse game state (required)
        GameStateData state_data = JsonProtocol::parse_game_state(request_json.at("game_state"));

        // Use default AI config - fast settings for single move
        AIConfig config;
        config.simulations = 1000;  // Reduced for faster response
        config.worlds = 20;
        config.epsilon = 0.1;
        config.use_threads = true;
        config.player_type = "safe_simple";

        // Allow optional overrides
        if (request_json.contains("simulations")) {
            config.simulations = request_json["simulations"].get<int>();
        }
        if (request_json.contains("player_type")) {
            config.player_type = request_json["player_type"].get<std::string>();
        }

        std::cout << "[DEBUG] Current player: " << state_data.current_player << std::endl;
        std::cout << "[DEBUG] Using fast defaults: sims=" << config.simulations
                  << ", type=" << config.player_type << std::endl;

        // Create game state
        game = new HeartsGameState(static_cast<int>(time(nullptr)));

        // Create the AI player
        player = create_player(config, nullptr);
        if (!player) {
            delete game;
            return JsonProtocol::format_error("AI_CONFIG_ERROR", "Failed to create AI player");
        }

        // Add players to game (AI player is always player 0)
        for (int i = 0; i < 4; i++) {
            if (i == 0) {
                game->addPlayer(player);
            } else {
                game->addPlayer(new Player(0));
            }
        }

        player->setGameState(game);
        game->Reset();

        // Set up the game state from input data
        // Only current player's hand is provided (always player 0)
        for (int p = 0; p < 4; p++) {
            game->cards[p].reset();
            game->original[p].reset();
        }
        // Set current player's hand (player 0)
        for (card c : state_data.player_hand) {
            game->cards[0].set(c);
            game->original[0].set(c);
        }

        game->setPassDir(state_data.pass_direction);
        // Set first player based on trick lead if there's a current trick,
        // otherwise default to player 0
        int first_player = state_data.current_trick_cards.empty() ? 0 : state_data.trick_lead_player;
        game->setFirstPlayer(first_player);
        game->setRules(state_data.rules);

        // setFirstPlayer with kLead2Clubs may override currPlr to 0 when passDir != kHold
        // We need to reset currPlr to the trick lead player before applying current trick moves
        if (!state_data.current_trick_cards.empty()) {
            game->currPlr = state_data.trick_lead_player;
        }

        for (int p = 0; p < 4; p++) {
            for (card c : state_data.played_cards[p]) {
                game->taken[p].set(c);
                game->allplayed.set(c);
            }
        }

        // Replay trick history: add cards to hands and apply moves
        for (const CompletedTrick& trick : state_data.trick_history) {
            // Set currPlr to the lead player for this trick
            game->currPlr = trick.lead_player;
            // First, add all cards in this trick to respective players' hands
            for (const TrickCard& tc : trick.cards) {
                game->cards[tc.player].set(tc.c);
                game->original[tc.player].set(tc.c);
            }
            // Then apply all moves in order
            for (const TrickCard& tc : trick.cards) {
                CardMove* move = new CardMove(tc.c, tc.player);
                game->ApplyMove(move);
                delete move;
            }
        }

        // For current trick cards, we need to add them to respective players' hands
        // before applying moves (ApplyMove checks if player has the card)
        for (const TrickCard& tc : state_data.current_trick_cards) {
            game->cards[tc.player].set(tc.c);
            game->original[tc.player].set(tc.c);
        }

        // Now apply the current trick moves
        for (const TrickCard& tc : state_data.current_trick_cards) {
            CardMove* move = new CardMove(tc.c, tc.player);
            game->ApplyMove(move);
            delete move;
        }

        // Get legal moves
        Move* legal_moves = game->getMoves();
        if (!legal_moves) {
            cleanup_player(player);
            delete game;
            return JsonProtocol::format_error("NO_LEGAL_MOVES", "No legal moves available");
        }

        // Count legal moves
        int num_moves = 0;
        for (Move* m = legal_moves; m; m = m->next) {
            num_moves++;
        }

        card move;
        if (num_moves == 1) {
            CardMove* card_move = dynamic_cast<CardMove*>(legal_moves);
            move = card_move->c;
            std::cout << "[DEBUG] Single legal move, skipping AI" << std::endl;
        } else {
            std::cout << "[DEBUG] Running AI for " << num_moves << " options..." << std::endl;
            move = compute_ai_move(game, player);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        std::cout << "[DEBUG] Chosen move: " << card_to_string(move) << std::endl;
        std::cout << "[DEBUG] Computation time: " << time_ms << " ms" << std::endl;
        std::cout << "============================================\n" << std::endl;

        // Format response (always player 0)
        std::string response = JsonProtocol::format_move_response(move, 0, time_ms);

        if (player) cleanup_player(player);
        delete game;

        return response;

    } catch (const json::exception& e) {
        if (player) cleanup_player(player);
        if (game) delete game;
        return JsonProtocol::format_error("PARSE_ERROR", std::string("JSON parse error: ") + e.what());
    } catch (const std::exception& e) {
        if (player) cleanup_player(player);
        if (game) delete game;
        return JsonProtocol::format_error("INTERNAL_ERROR", std::string("Internal error: ") + e.what());
    } catch (...) {
        if (player) cleanup_player(player);
        if (game) delete game;
        return JsonProtocol::format_error("UNKNOWN_ERROR", "An unknown error occurred");
    }
}

void AIRequestHandler::cleanup_player(Player* player) {
    // Note: Don't delete the player itself - it's in the game's player list
    // and will be deleted when the game is deleted.
    // We only need to delete the algorithm which the player owns.
    if (player) {
        Algorithm* alg = player->getAlgorithm();
        if (alg) {
            delete alg;
        }
    }
}

} // namespace server
} // namespace hearts
