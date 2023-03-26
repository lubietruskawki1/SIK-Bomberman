#ifndef GAME_H
#define GAME_H

#include <map>
#include <set>
#include <utility>
#include "buffer.h"

class Player {
private:
    std::string name;
    std::string address;

public:
    Player() = default;
    explicit Player(Buffer &buffer) {
        buffer.get(name);
        buffer.get(address);
    }
    Player(std::string name, std::string address) :
            name(std::move(name)), address(std::move(address)) {}

    void insert_to_buffer(Buffer &buffer) const {
        buffer.insert(name);
        buffer.insert(address);
    }
};

class Position {
private:
    coordinate_t x{};
    coordinate_t y{};

public:
    Position() = default;
    explicit Position(Buffer &buffer) {
        buffer.get(x, COORDINATE_SIZE);
        buffer.get(y, COORDINATE_SIZE);
    }
    Position(coordinate_t x, coordinate_t y) : x(x), y(y) {}

    inline bool operator<(const Position &that) const {
        return x != that.x ? x < that.x : y < that.y;
    }

    inline bool operator!=(const Position &that) const {
        return x != that.x || y != that.y;
    }

    void insert_to_buffer(Buffer &buffer) const {
        buffer.insert(x, COORDINATE_SIZE);
        buffer.insert(y, COORDINATE_SIZE);
    }

    [[nodiscard]] coordinate_t get_x() const {
        return x;
    }

    [[nodiscard]] coordinate_t get_y() const {
        return y;
    }
};

class Bomb {
private:
    Position position;
    bomb_timer_t timer{};

public:
    Bomb() = default;
    explicit Bomb(Buffer &buffer) {
        Position position_copy(buffer);
        position = position_copy;
        buffer.get(timer, BOMB_TIMER_SIZE);
    }
    Bomb(Position position, bomb_timer_t timer) : position(position), timer(timer) {}

    void decrease_timer() {
        timer--;
    }

    void insert_to_buffer(Buffer &buffer) const {
        position.insert_to_buffer(buffer);
        buffer.insert(timer, BOMB_TIMER_SIZE);
    }

    Position get_position() {
        return position;
    }

    [[nodiscard]] bomb_timer_t get_timer() const {
        return timer;
    }
};

enum class GameStateType : message_id_t {
    Lobby,
    Game,
};

struct GameState {
    // Common attributes.
    GameStateType type{};
    std::mutex mutex; // For safety.
    std::string server_name;
    players_count_t players_count{};
    coordinate_t size_x{};
    coordinate_t size_y{};
    game_length_t game_length{};
    explosion_radius_t explosion_radius{};
    bomb_timer_t bomb_timer{};
    turn_t turn{};
    std::map<player_id_t, Player> players;
    std::map<player_id_t, Position> player_positions;
    std::set<Position> blocks;
    std::map<bomb_id_t, Bomb> bombs;
    std::set<Position> explosions;
    std::map<player_id_t, score_t> scores;
    // Attributes used by server only.
    turn_duration_t turn_duration{};
    initial_blocks_t initial_blocks{};
    seed_t seed{};
};

// Id number generator used by server for clients, players and bombs.
template <typename T>
class IdGenerator {
private:
    T next_id;
public:
    IdGenerator(): next_id(0) {}

    // Returns the next id.
    T generate_id() {
        return next_id++;
    }

    void reset() {
        next_id = 0;
    }
};

#endif //GAME_H
