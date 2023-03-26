#ifndef PROGRAM_OPTIONS_H
#define PROGRAM_OPTIONS_H

#include <boost/program_options.hpp>
#include <string>
#include <stdexcept>
#include "types.h"

namespace po = boost::program_options;

constexpr port_t PORT_MAX = UINT16_MAX;
constexpr char NULL_TERMINATOR = '\0';
constexpr int DECIMAL_BASE = 10;

po::options_description get_client_options_description() {
    po::options_description client_options_description("Client parameters");

    client_options_description.add_options()(
        "help,h", "Produce help message"
    )("gui-address,d", po::value<std::string>()->required(),
        "Address of the GUI server <(host name):(port) or (IPv4):(port) or (IPv6):(port)>"
    )("player-name,n", po::value<std::string>()->required(),
        "Player nickname in the game"
    )("port,p", po::value<port_parsing_t>()->required(),
        "Port on which the client is listening for messages from GUI"
    )("server-address,s", po::value<std::string>()->required(),
        "Address of the game server <(host name):(port) or (IPv4):(port) or (IPv6):(port)>");

    return client_options_description;
}

po::options_description get_server_options_description() {
    po::options_description server_options_description("Server parameters");

    server_options_description.add_options()(
        "help,h", "Produce help message"
    )("bomb-timer,b", po::value<bomb_timer_parsing_t>()->required(),
        "Number of turns after which a bomb explodes"
    )("players-count,c", po::value<players_count_parsing_t>()->required(),
        "Number of players"
    )("turn-duration,d", po::value<turn_duration_parsing_t>()->required(),
        "Duration of one turn in milliseconds"
    )("explosion-radius,e", po::value<explosion_radius_parsing_t>()->required(),
        "Radius of bomb explosions"
    )("initial-blocks,k", po::value<initial_blocks_parsing_t>()->required(),
        "Initial number of blocks on the board"
    )("game-length,l", po::value<game_length_parsing_t>()->required(),
        "Length of the game in turns"
    )("server-name,n", po::value<std::string>()->required(),
        "Name of the server"
    )("port,p", po::value<port_parsing_t>()->required(),
        "Port on which the server is listening for messages from clients"
    )("seed,s", po::value<seed_parsing_t>()->default_value(0),
        "Seed to be used for generating random values (default value is 0)"
    )("size-x,x", po::value<coordinate_parsing_t>()->required(),
        "Horizontal size of the board"
    )("size-y,y", po::value<coordinate_parsing_t>()->required(),
        "Vertical size of the board");

    return server_options_description;
}

po::variables_map get_variables_map(int argc, char **argv,
                                    const po::options_description &options_description) {
    po::variables_map variables_map;
    po::store(po::parse_command_line(argc, argv, options_description), variables_map);
    return variables_map;
}

template <typename T>
void throw_parsing_error(T value, const std::string &option) {
    std::stringstream error;
    error << "the argument ('" << value << "') for option '--" << option << "' is invalid";
    throw po::error(error.str());
}

uint8_t parse(int16_t value, const std::string &option) {
    if (value < 0 || value > UINT8_MAX) {
        throw_parsing_error(value, option);
    }
    return (uint8_t) value;
}

uint16_t parse(int32_t value, const std::string &option) {
    if (value < 0 || value > UINT16_MAX) {
        throw_parsing_error(value, option);
    }
    return (uint16_t) value;
}

uint32_t parse(int64_t value, const std::string &option) {
    if (value < 0 || value > UINT32_MAX) {
        throw_parsing_error(value, option);
    }
    return (uint32_t) value;
}

uint64_t parse(const std::string &string, const std::string &option) {
    errno = 0;
    char *end_ptr;
    uint64_t value = strtoull(string.c_str(), &end_ptr, DECIMAL_BASE);
    if (errno != 0 || *end_ptr != NULL_TERMINATOR || string[0] == '-') {
        throw_parsing_error(string, option);
    }
    return (uint64_t) value;
}

void notify_variables_map(po::variables_map &variables_map) {
    po::notify(variables_map);
}

struct Address {
    std::string host;
    std::string port;
};

// Checks if 'string' describes a valid port number.
bool is_valid_port_number(const std::string &string) {
    errno = 0;
    char *end_ptr;
    unsigned long port = strtoul(string.c_str(), &end_ptr, DECIMAL_BASE);
    if (errno != 0 || port > PORT_MAX || *end_ptr != NULL_TERMINATOR) {
        return false;
    }
    return true;
}

Address parse_address(const std::string &address) {
    std::string::size_type colon_position = address.rfind(':');
    if (colon_position == std::string::npos) {
        throw std::invalid_argument("Address " + address + " missing a colon");
    }
    std::string host = address.substr(0, colon_position);
    std::string port = address.substr(colon_position + 1);
    if (host.empty()) {
        throw std::invalid_argument("Address " + address + " missing a host");
    }
    if (port.empty()) {
        throw std::invalid_argument("Address " + address + " missing a port");
    }
    if (!is_valid_port_number(port)) {
        throw std::invalid_argument(port + " is not a valid port number");
    }
    return {host, port};
}

#endif //PROGRAM_OPTIONS_H