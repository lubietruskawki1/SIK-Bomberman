#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include <utility>

#include "client_connection.h"
#include "game_manager.h"

class Server {
private:
    GameState &game_state;
    std::map<client_id_t, ClientConnection> clients;
    IdGenerator<client_id_t> client_id_generator;
    std::map<client_id_t, player_id_t> client_to_player_id;
    BlockingMessageQueue pending_messages; // Messages to be sent to clients.
    GameManager game_manager;

    void remove_closed_connections();
    void send_message_to_clients();
    std::map<player_id_t, ClientMessage> collect_last_messages();
    void reset_last_messages();

    void collect_players() {
        bool players_collected = false;
        {
            std::lock_guard <std::mutex> lock(game_state.mutex);
            if (game_state.players_count == game_state.players.size()) {
                players_collected = true;
            }
        }
        while (!players_collected) {
            send_message_to_clients();
            {
                std::lock_guard <std::mutex> lock(game_state.mutex);
                if (game_state.players_count == game_state.players.size()) {
                    players_collected = true;
                }
            }
        }
    }

    void start_game() {
        game_manager.start_game();
        send_message_to_clients(); // GameStarted.
    }

    void initialize_game_state() {
        game_manager.initialize_game_state();
        send_message_to_clients(); // Turn.
    }

    void run_turn(turn_t turn, std::map<player_id_t, ClientMessage> last_messages) {
        game_manager.run_turn(turn, std::move(last_messages));
        send_message_to_clients(); // Turn.
    }

    void end_game() {
        game_manager.end_game();
        send_message_to_clients(); // GameEnded.
    }

    void reset_game_state() {
        game_manager.reset_game_state();
    }

public:
    explicit Server(GameState &game_state) :
            game_state(game_state), game_manager(game_state, client_to_player_id, pending_messages) {}

    void accept_clients(as::io_context &io_context, port_t port);
    void run_game();
};

#endif //SERVER_MANAGER_H
