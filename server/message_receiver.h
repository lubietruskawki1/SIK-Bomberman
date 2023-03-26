#ifndef MESSAGE_RECEIVER_H
#define MESSAGE_RECEIVER_H

#include "game_manager.h"
#include <utility>

namespace as = boost::asio;

// Receives messages from client.
class MessageReceiver {
private:
    std::shared_ptr<as::ip::tcp::socket> client_socket;
    std::string client_address;
    client_id_t client_id;
    GameManager &game_manager;
    ClientMessage &newest_message;
    bool &new_message;
    std::mutex &new_message_mutex;

public:
    explicit MessageReceiver(std::shared_ptr<as::ip::tcp::socket> client_socket,
                             std::string client_address, client_id_t client_id,
                             GameManager &game_manager, ClientMessage &newest_message,
                             bool &new_message, std::mutex &new_message_mutex) :
            client_socket(std::move(client_socket)), client_address(std::move(client_address)),
            client_id(client_id), game_manager(game_manager), newest_message(newest_message),
            new_message(new_message), new_message_mutex(new_message_mutex) {}

    void receive_messages() {
        try {
            BufferTCP buffer(*client_socket);
            do {
                ClientMessage client_message(buffer);
                if (!client_message.is_correct()) {
                    // Client needs to be disconnected.
                    throw std::invalid_argument("Incorrect message from client");
                }
                switch (client_message.get_type()) {
                    case ClientMessageType::Join: {
                        Player player(client_message.get_player_name(), client_address);
                        game_manager.add_player(player, client_id);
                        break;
                    }
                    case ClientMessageType::PlaceBomb:
                    case ClientMessageType::PlaceBlock:
                    case ClientMessageType::Move: {
                        {
                            std::lock_guard<std::mutex> lock(new_message_mutex);
                            newest_message = client_message;
                            new_message = true;
                        }
                        break;
                    }
                }
            } while (true);
        } catch (std::exception &e) {
            throw;
        } catch (...) {
            throw;
        }
    }
};

#endif //MESSAGE_RECEIVER_H
