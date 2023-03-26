#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include "../common/program_options.h"
#include "../common/messages.h"
#include "server.h"

namespace po = boost::program_options;
namespace as = boost::asio;

// Prepare the state of the game.
void parse_variables_map(const po::variables_map &variables_map, GameState &game_state,
                         port_t &port) {
    port = parse(variables_map["port"].as<port_parsing_t>(), "port");
    game_state.bomb_timer = parse(
            variables_map["bomb-timer"].as<bomb_timer_parsing_t>(), "bomb-timer");
    game_state.players_count = parse(
            variables_map["players-count"].as<players_count_parsing_t>(), "players-count");
    game_state.turn_duration = parse(
            variables_map["turn-duration"].as<turn_duration_parsing_t>(), "turn-duration");
    game_state.explosion_radius = parse(
            variables_map["explosion-radius"].as<explosion_radius_parsing_t>(), "explosion-radius");
    game_state.initial_blocks = parse(
            variables_map["initial-blocks"].as<initial_blocks_parsing_t>(), "initial-blocks");
    game_state.game_length = parse(
            variables_map["game-length"].as<game_length_parsing_t>(), "game-length");
    game_state.server_name = variables_map["server-name"].as<std::string>();
    game_state.seed = parse(
            variables_map["seed"].as<seed_parsing_t>(), "seed");
    game_state.size_x = parse(
            variables_map["size-x"].as<coordinate_parsing_t>(), "size-x");
    game_state.size_y = parse(
            variables_map["size-y"].as<coordinate_parsing_t>(), "size-y");
}

int main(int argc, char **argv) {
    try {
        as::io_context io_context;

        po::options_description server_options_description = get_server_options_description();

        po::variables_map variables_map = get_variables_map(argc, argv, server_options_description);

        if (variables_map.count("help")) {
            std::cout << server_options_description;
            return 0;
        }

        notify_variables_map(variables_map);

        GameState game_state;
        game_state.type = GameStateType::Lobby;
        port_t port;
        parse_variables_map(variables_map, game_state, port);

        Server server(game_state);
        // Start thread responsible for accepting new connections.
        std::thread thread_accepting_clients(
                [&server, &io_context, &port] {
                    server.accept_clients(io_context, port);
                });
        try {
            // Run games in a loop.
            do {
                server.run_game();
            } while (true);
        } catch (std::exception &e) {
            thread_accepting_clients.join();
        }
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "Error: Exception of unknown type\n";
        exit(EXIT_FAILURE);
    }
}