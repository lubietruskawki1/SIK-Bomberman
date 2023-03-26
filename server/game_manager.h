#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <utility>
#include <random>
#include "blocking_queue.h"

constexpr turn_t TURN_ZERO = 0;

class GameManager {
private:
    GameState &game_state;
    IdGenerator<player_id_t> player_id_generator;
    std::map<client_id_t, player_id_t> &client_to_player_id;
    // If server is in Lobby state 'past_messages' contains a Hello message and all sent
    // AcceptedPlayer messages. If it is in Game state, it contains a Hello message and all sent
    // Turn messages.
    std::queue<std::shared_ptr<ServerMessage>> past_messages;
    std::mutex past_messages_mutex;
    BlockingMessageQueue &pending_messages;
    std::minstd_rand random;
    IdGenerator<bomb_id_t> bomb_id_generator;

    void reset_past_messages();
    Position get_random_position();

    void process_bombs(std::vector<std::shared_ptr<Event>> &events,
                       std::set<player_id_t> &current_turn_robots_destroyed);
    void process_place_bomb(std::vector<std::shared_ptr<Event>> &events, player_id_t player_id);
    void process_place_block(std::vector<std::shared_ptr<Event>> &events, player_id_t player_id);
    void process_move(std::vector<std::shared_ptr<Event>> &events, player_id_t player_id,
                      direction_t direction);
    void process_player_moves(std::vector<std::shared_ptr<Event>> &events,
                              std::map<player_id_t, ClientMessage> current_turn_messages,
                              const std::set<player_id_t>& current_turn_robots_destroyed);

public:
    explicit GameManager(GameState &game_state,
                         std::map<client_id_t, player_id_t> &client_to_player_id,
                         BlockingMessageQueue &pending_messages) :
            game_state(game_state), client_to_player_id(client_to_player_id),
            pending_messages(pending_messages), random(game_state.seed) {
        reset_past_messages();
    }

    std::shared_ptr<BlockingMessageQueue> get_past_messages() {
        std::lock_guard<std::mutex> lock(past_messages_mutex);
        return std::make_shared<BlockingMessageQueue>(past_messages);
    }

    void add_past_message(const std::shared_ptr<ServerMessage> &message) {
        std::lock_guard<std::mutex> lock(past_messages_mutex);
        past_messages.push(message);
    }

    void add_player(const Player &player, client_id_t client_id);
    void start_game();
    void initialize_game_state();
    void run_turn(turn_t turn, std::map<player_id_t, ClientMessage> current_turn_messages);
    void end_game();
    void reset_game_state();
};

#endif //GAME_MANAGER_H
