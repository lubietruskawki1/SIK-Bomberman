#ifndef EVENTS_H
#define EVENTS_H

#include "game.h"

enum class EventType : event_type_t {
    BombPlaced = 0,
    BombExploded = 1,
    PlayerMoved = 2,
    BlockPlaced = 3,
};

class Event {
protected:
    EventType type{};
public:
    Event() = default;
    explicit Event(EventType type) : type(type) {}
    virtual ~Event() = default;
    virtual void execute([[maybe_unused]] GameState &game_state) {}
    virtual void execute([[maybe_unused]] GameState &game_state,
                         [[maybe_unused]] std::set<player_id_t> &destroyed_players,
                         [[maybe_unused]] std::set<Position> &destroyed_blocks) {}
    virtual void insert_to_buffer([[maybe_unused]] Buffer &buffer) const {}
};

class BombPlaced : public Event {
private:
    bomb_id_t id{};
    Position position{};

public:
    explicit BombPlaced(Buffer &buffer);
    explicit BombPlaced(bomb_id_t id, Position position)
            : Event(EventType::BombPlaced), id(id), position(position) {}
    void execute(GameState &game_state) override;
    void insert_to_buffer([[maybe_unused]] Buffer &buffer) const override;
};

class BombExploded : public Event {
private:
    bomb_id_t id{};
    std::set<player_id_t> robots_destroyed;
    std::set<Position> blocks_destroyed;

    void calculate_explosion(GameState &game_state) const;
    void calculate_robots_destroyed(GameState &game_state);
    void calculate_blocks_destroyed(GameState &game_state);
    void update_destroyed_robots_and_blocks(std::set<player_id_t> &destroyed_robots,
                                            std::set<Position> &destroyed_blocks);

public:
    explicit BombExploded(Buffer &buffer);
    explicit BombExploded(bomb_id_t id, GameState &game_state,
                          std::set<player_id_t> &destroyed_robots,
                          std::set<Position> &destroyed_blocks);
    void execute(GameState &game_state, std::set<player_id_t> &destroyed_robots,
                 std::set<Position> &destroyed_blocks) override;
    void insert_to_buffer([[maybe_unused]] Buffer &buffer) const override;
};

class PlayerMoved : public Event {
private:
    player_id_t id{};
    Position position{};

public:
    explicit PlayerMoved(Buffer &buffer);
    PlayerMoved(player_id_t id, Position position) :
            Event(EventType::PlayerMoved), id(id), position(position) {}
    void execute(GameState &game_state) override;
    void insert_to_buffer(Buffer &buffer) const override;
};

class BlockPlaced : public Event {
private:
    Position position{};

public:
    explicit BlockPlaced(Buffer &buffer);
    explicit BlockPlaced(Position position) : Event(EventType::BlockPlaced), position(position) {}
    void execute(GameState &game_state) override;
    void insert_to_buffer(Buffer &buffer) const override;
};

#endif //EVENTS_H
