#ifndef MESSAGES_H
#define MESSAGES_H

#include <utility>
#include "events.h"

enum class Direction : direction_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

enum class InputMessageType : message_id_t {
    PlaceBomb = 0,
    PlaceBlock = 1,
    Move = 2
};

enum class ClientMessageType : message_id_t {
    Join = 0,
    PlaceBomb = 1,
    PlaceBlock = 2,
    Move = 3
};

class InputMessage {
private:
    InputMessageType type{};
    direction_t direction{};
    bool ignore = false; // Client ignores incorrect messages from GUI.

public:
    explicit InputMessage(Buffer &buffer);
    InputMessageType get_type() {
        return type;
    }
    [[nodiscard]] direction_t get_direction() const {
        return direction;
    }
    [[nodiscard]] bool should_be_ignored() const {
        return ignore;
    }
};

class ClientMessage {
private:
    ClientMessageType type{};
    std::string player_name; // For Join message.
    direction_t direction{}; // For Move message.
    bool correct = true;

public:
    ClientMessage() = default;
    explicit ClientMessage(InputMessage input_message, GameState &game_state,
                           std::string name);
    explicit ClientMessage(Buffer &buffer);
    void insert_to_buffer(Buffer &buffer) const;
    ClientMessageType get_type() {
        return type;
    }
    [[nodiscard]] direction_t get_direction() const {
        return direction;
    }
    [[nodiscard]] bool is_correct() const {
        return correct;
    }
    std::string get_player_name() {
        return player_name;
    }
};

enum class ServerMessageType : message_id_t {
    Hello = 0,
    AcceptedPlayer = 1,
    GameStarted = 2,
    Turn = 3,
    GameEnded = 4
};

class ServerMessage {
private:
    ServerMessageType type{};
    bool send_to_gui = true; // After receiving a GameStarted message don't send a message to GUI.
    player_id_t accepted_player_id{}; // For PlayerAccepted message.
    Player accepted_player; // For PlayerAccepted message.
    std::map<player_id_t, Player> players; // For GameStarted message.
    turn_t turn{}; // For Turn message.
    std::set<player_id_t> robots_destroyed; // For Turn message.
    std::set<Position> blocks_destroyed; // For Turn message.
    std::vector<std::shared_ptr<Event>> events; // For Turn message.
    std::map<player_id_t, score_t> scores; // For GameEnded message.

public:
    ServerMessage() = default;

    // Hello message constructor.
    explicit ServerMessage(ServerMessageType type) : type(type) {}

    // PlayerAccepted message constructor.
    explicit ServerMessage(ServerMessageType type, player_id_t player_id, Player player) :
            type(type), accepted_player_id(player_id), accepted_player(player) {}

    // GameStarted and GameEnded message constructor.
    explicit ServerMessage(ServerMessageType type, GameState &game_state) :
            type(type), players(game_state.players), scores(game_state.scores) {}

    // Turn message constructor.
    explicit ServerMessage(ServerMessageType type, turn_t turn,
                           std::vector<std::shared_ptr<Event>> events) :
            type(type), turn(turn), events(std::move(events)) {}

    explicit ServerMessage(Buffer &buffer, GameState &game_state);
    void insert_to_buffer(Buffer &buffer, GameState &game_state) const;
    ServerMessageType get_type() {
        return type;
    }
    [[nodiscard]] bool should_send_message_to_gui() const {
        return send_to_gui;
    }
};

enum class DrawMessageType : message_id_t {
    Lobby = 0,
    Game = 1,
};

class DrawMessage {
private:
    DrawMessageType type{};

public:
    explicit DrawMessage(ServerMessage server_message, GameState &game_state);
    void insert_to_buffer(Buffer &buffer, GameState &game_state) const;
};

#endif //MESSAGES_H
