#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include "../common/program_options.h"
#include "../common/messages.h"

namespace po = boost::program_options;
namespace as = boost::asio;

// Receives messages from GUI and forwards appropriate ones to server.
void from_gui_to_server(as::ip::udp::socket &gui_socket, as::ip::udp::endpoint &gui_endpoint,
                        as::ip::tcp::socket &server_socket, GameState &game_state,
                        const std::string &player_name) {
    try {
        BufferUDP gui_buffer(gui_socket, gui_endpoint);
        BufferTCP server_buffer(server_socket);
        do {
            gui_buffer.receive_message();
            InputMessage input_message(gui_buffer);
            if (!input_message.should_be_ignored()) {
                ClientMessage client_message(input_message, game_state, player_name);
                client_message.insert_to_buffer(server_buffer);
                server_buffer.send_message();
            }
        } while (true);
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "Error: Exception of unknown type\n";
        exit(EXIT_FAILURE);
    }
}

// Receives messages from server and forwards appropriate ones to GUI.
void from_server_to_gui(as::ip::udp::socket &gui_socket, as::ip::udp::endpoint &gui_endpoint,
                        as::ip::tcp::socket &server_socket, GameState &game_state) {
    try {
        BufferUDP gui_buffer(gui_socket, gui_endpoint);
        BufferTCP server_buffer(server_socket);
        do {
            ServerMessage server_message(server_buffer, game_state);
            if (server_message.should_send_message_to_gui()) {
                DrawMessage draw_message(server_message, game_state);
                draw_message.insert_to_buffer(gui_buffer, game_state);
                gui_buffer.send_message();
            }
        } while (true);
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "Error: Exception of unknown type\n";
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    try {
        as::io_context io_context;
        as::ip::tcp::resolver tcp_resolver(io_context);
        as::ip::udp::resolver udp_resolver(io_context);
        as::ip::udp::endpoint gui_endpoint{};
        as::ip::tcp::resolver::results_type server_endpoint{};
        boost::system::error_code error;

        po::options_description client_options_description = get_client_options_description();

        po::variables_map variables_map = get_variables_map(argc, argv, client_options_description);

        if (variables_map.count("help")) {
            std::cout << client_options_description;
            return 0;
        }

        notify_variables_map(variables_map);

        // Parse variables map.
        port_t port = parse(variables_map["port"].as<port_parsing_t>(), "port");

        // Prepare the state of the game.
        GameState game_state;
        game_state.type = GameStateType::Lobby;

        std::string player_name = variables_map["player-name"].as<std::string>();

        Address server_address = parse_address(variables_map["server-address"].as<std::string>());
        server_endpoint = tcp_resolver.resolve(server_address.host, server_address.port);

        Address gui_address = parse_address(variables_map["gui-address"].as<std::string>());
        gui_endpoint = *udp_resolver.resolve(gui_address.host, gui_address.port);

        // UDP connection (with GUI).
        as::ip::udp::socket gui_socket(io_context, as::ip::udp::endpoint(as::ip::udp::v6(), port));

        // TCP connection (with server).
        as::ip::tcp::socket server_socket(io_context);
        as::connect(server_socket, server_endpoint, error);
        server_socket.set_option(as::ip::tcp::no_delay(true));

        std::thread thread_from_gui_to_server(
                [&gui_socket, &gui_endpoint, &server_socket, &game_state, &player_name] {
                    from_gui_to_server(gui_socket, gui_endpoint, server_socket, game_state,
                                       player_name);
                });

        std::thread thread_from_server_to_gui(
                [&gui_socket, &gui_endpoint, &server_socket, &game_state] {
                    from_server_to_gui(gui_socket, gui_endpoint, server_socket, game_state);
                });

        thread_from_gui_to_server.join();
        thread_from_server_to_gui.join();
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "Error: Exception of unknown type\n";
        exit(EXIT_FAILURE);
    }
}