#ifndef CLIENT_H
#define CLIENT_H

#include <utility>
#include "message_sender.h"
#include "message_receiver.h"

class ClientConnection {
private:
    std::shared_ptr<as::ip::tcp::socket> client_socket;
    std::string client_address;
    std::thread thread_client;
    MessageSender message_sender;
    MessageReceiver message_receiver;
    ClientMessage newest_message;
    bool new_message = false;
    std::mutex new_message_mutex;
    bool closed = false;
    std::mutex closed_mutex;

public:
    explicit ClientConnection(const std::shared_ptr<as::ip::tcp::socket> &client_socket,
                              std::string client_address, client_id_t client_id,
                              GameState &game_state, GameManager &game_manager) :
            client_socket(client_socket), client_address(std::move(client_address)),
            message_sender(client_socket, game_state, game_manager),
            message_receiver(client_socket, this->client_address, client_id, game_manager,
                             newest_message, new_message, new_message_mutex) {}

    ~ClientConnection() {
        if (thread_client.joinable()) {
            thread_client.join();
        }
    }

    void send_and_receive_messages() {
        std::thread thread_message_sender(&MessageSender::send_messages, &message_sender);
        try {
            message_receiver.receive_messages();
        } catch (std::exception &e) {
            // Close connection.
            message_sender.close_connection();
            if (thread_message_sender.joinable()) {
                thread_message_sender.join();
            }
            // Close socket.
            try {
                client_socket->shutdown(as::ip::tcp::socket::shutdown_both);
            } catch (std::exception &e) {
                // Ignore.
            }
            client_socket->close();
            {
                // Mark connection as closed.
                std::lock_guard<std::mutex> lock(closed_mutex);
                closed = true;
            }
        }
    }

    void set_thread_client(std::thread &&thread) {
        thread_client = std::move(thread);
    }

    // Returns true if client sent a new message since the last reset.
    bool has_a_new_message() {
        std::lock_guard<std::mutex> lock(new_message_mutex);
        return new_message;
    }

    ClientMessage get_newest_message() {
        return newest_message;
    }

    void reset_newest_message() {
        std::lock_guard<std::mutex> lock(new_message_mutex);
        new_message = false;
    }

    [[nodiscard]] bool is_closed() {
        std::lock_guard<std::mutex> lock(closed_mutex);
        return closed;
    }

    void send_message(const std::shared_ptr<ServerMessage> &server_message) {
        if (!is_closed()) {
            message_sender.send_message(server_message);
        }
    }
};

#endif //CLIENT_H
