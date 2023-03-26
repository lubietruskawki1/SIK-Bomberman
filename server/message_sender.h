#ifndef MESSAGE_SENDER_H
#define MESSAGE_SENDER_H

#include <utility>
#include "blocking_queue.h"
#include "game_manager.h"

namespace as = boost::asio;

// Sends messages to client.
class MessageSender {
private:
    std::shared_ptr<as::ip::tcp::socket> client_socket;
    std::shared_ptr<BlockingMessageQueue> messages;
    GameState &game_state;

public:
    explicit MessageSender(std::shared_ptr<as::ip::tcp::socket> client_socket,
                           GameState &game_state, GameManager &game_manager) :
            client_socket(std::move(client_socket)), game_state(game_state) {
        messages = game_manager.get_past_messages();
    }

    void send_messages() {
        try {
            BufferTCP buffer(*client_socket);
            do {
                std::shared_ptr<ServerMessage> server_message = messages->pop();
                server_message->insert_to_buffer(buffer, game_state);
                buffer.send_message();
            } while (true);
        } catch (std::exception &e) {
            close_connection();
        } catch (...) {
            close_connection();
        }
    }

    void close_connection() {
        try {
            client_socket->shutdown(as::ip::tcp::socket::shutdown_both);
        } catch (std::exception &e) {
            // Ignore.
        }
        client_socket->close();
        messages->close_client_connection();
    }

    void send_message(const std::shared_ptr<ServerMessage> &server_message) {
        messages->push(server_message);
    }
};

#endif //MESSAGE_SENDER_H
