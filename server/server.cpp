#include "server.h"

void Server::remove_closed_connections() {
    std::set<client_id_t> clients_to_be_removed;
    for (auto & [client_id, client_connection]: clients) {
        if (client_connection.is_closed()) {
            clients_to_be_removed.insert(client_id);
        }
    }
    for (auto & client_id: clients_to_be_removed) {
        clients.erase(client_id);
    }
}

void Server::send_message_to_clients() {
    std::shared_ptr<ServerMessage> server_message = pending_messages.pop();
    game_manager.add_past_message(server_message);
    remove_closed_connections();
    for (auto & [client_id, client_connection]: clients) {
        if (!client_connection.is_closed()) {
            client_connection.send_message(server_message);
        }
    }
}

std::map<player_id_t, ClientMessage> Server::collect_last_messages() {
    std::map<player_id_t, ClientMessage> last_messages;
    for (auto & [client_id, client]: clients) {
        if (client_to_player_id.contains(client_id) && client.has_a_new_message()) {
            player_id_t player_id = client_to_player_id[client_id];
            last_messages[player_id] = client.get_newest_message();
        }
    }
    return last_messages;
}

void Server::reset_last_messages() {
    for (auto & [client_id, client_connection]: clients) {
        if (client_connection.has_a_new_message()) {
            client_connection.reset_newest_message();
        }
    }
}

void Server::accept_clients(as::io_context &io_context, port_t port) {
    as::ip::tcp::acceptor tcp_acceptor(io_context,
                                       as::ip::tcp::endpoint(as::ip::tcp::v6(), port));
    do {
        auto client_socket = std::make_shared<as::ip::tcp::socket>(io_context);
        tcp_acceptor.accept(*client_socket);
        (*client_socket).set_option(as::ip::tcp::no_delay(true));
        std::ostringstream client_address;
        client_address << (*client_socket).remote_endpoint();
        client_id_t client_id = client_id_generator.generate_id();
        auto [iterator, insertion_success] =
                clients.try_emplace(client_id, client_socket, client_address.str(),
                                    client_id, game_state, game_manager);
        ClientConnection &client_connection = iterator->second;
        std::thread thread_client(
                [&client_connection] {
                    client_connection.send_and_receive_messages();
                });
        client_connection.set_thread_client(std::move(thread_client));
    } while (true);
}

void Server::run_game() {
    collect_players();
    start_game();
    initialize_game_state();
    reset_last_messages();
    for (turn_t turn = 1; turn <= game_state.game_length; turn++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(game_state.turn_duration));
        std::map<player_id_t, ClientMessage> last_messages = collect_last_messages();
        run_turn(turn, last_messages);
        reset_last_messages();
    }
    end_game();
    reset_game_state();
}