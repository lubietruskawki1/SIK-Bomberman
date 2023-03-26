#include "events.h"

// Gets a BombPlaced event from buffer.
BombPlaced::BombPlaced(Buffer &buffer) {
    buffer.get(id, BOMB_ID_SIZE);
    Position position_copy(buffer);
    position = position_copy;
}

// Executes the BombPlaced event and updates 'game_state' and 'destroyed_players' and
// 'destroyed_blocks' sets accordingly.
void BombPlaced::execute(GameState &game_state) {
    // Insert a new bomb.
    game_state.bombs[id] = Bomb(position, game_state.bomb_timer);
}

void BombPlaced::insert_to_buffer([[maybe_unused]] Buffer &buffer) const {
    auto event_type = static_cast<message_id_t>(type);
    buffer.insert(event_type, EVENT_TYPE_SIZE);
    buffer.insert(id, BOMB_ID_SIZE);
    position.insert_to_buffer(buffer);
}

// Gets a BombExploded event from buffer.
BombExploded::BombExploded(Buffer &buffer) {
    buffer.get(id, BOMB_ID_SIZE);

    list_length_t robots_destroyed_size;
    buffer.get(robots_destroyed_size, LIST_LENGTH_SIZE);
    for (list_length_t i = 0; i < robots_destroyed_size; i++) {
        player_id_t player_id;
        buffer.get(player_id, PLAYER_ID_SIZE);
        robots_destroyed.insert(player_id);
    }

    list_length_t blocks_destroyed_size;
    buffer.get(blocks_destroyed_size, LIST_LENGTH_SIZE);
    for (list_length_t i = 0; i < blocks_destroyed_size; i++) {
        Position position(buffer);
        blocks_destroyed.insert(position);
    }
}

void BombExploded::calculate_explosion(GameState &game_state) const {
    Position bomb_position(game_state.bombs[id].get_position());
    coordinate_t bomb_position_x = bomb_position.get_x();
    coordinate_t bomb_position_y = bomb_position.get_y();

    // Add fields on the left side of the bomb to explosions.
    int left = std::max((int) bomb_position_x - (int) game_state.explosion_radius, 0);
    for (int x = (int) bomb_position_x; x >= left; x--) {
        Position explosion((coordinate_t) x, bomb_position_y);
        game_state.explosions.insert(explosion);
        // Exit the loop when we reached a position containing a block.
        if (game_state.blocks.contains(explosion)) {
            break;
        }
    }

    // Add fields on the right side of the bomb to explosions.
    int right = std::min((int) (bomb_position_x) + (int) game_state.explosion_radius,
                         (int) (game_state.size_x - 1));
    for (int x = bomb_position_x; x <= right; x++) {
        Position explosion((coordinate_t) x, bomb_position_y);
        game_state.explosions.insert(explosion);
        // Exit the loop when we reached a position containing a block.
        if (game_state.blocks.contains(explosion)) {
            break;
        }
    }

    // Add fields under the bomb to explosions.
    int bottom = std::max((int) bomb_position_y - (int) game_state.explosion_radius, 0);
    for (int y = bomb_position_y; y >= bottom; y--) {
        Position explosion(bomb_position_x, (coordinate_t) y);
        game_state.explosions.insert(explosion);
        // Exit the loop when we reached a position containing a block.
        if (game_state.blocks.contains(explosion)) {
            break;
        }
    }

    // Add fields above the bomb to explosions.
    int top = std::min((int) (bomb_position_y) + (int) game_state.explosion_radius,
                       (int) (game_state.size_y - 1));
    for (int y = bomb_position_y; y <= top; y++) {
        Position explosion(bomb_position_x, (coordinate_t) y);
        game_state.explosions.insert(explosion);
        // Exit the loop when we reached a position containing a block.
        if (game_state.blocks.contains(explosion)) {
            break;
        }
    }
}

void BombExploded::calculate_robots_destroyed(GameState &game_state) {
    for (auto & [player_id, position]: game_state.player_positions) {
        if (game_state.explosions.contains(position)) {
            robots_destroyed.insert(player_id);
        }
    }
}

void BombExploded::calculate_blocks_destroyed(GameState &game_state) {
    for (auto & position: game_state.blocks) {
        if (game_state.explosions.contains(position)) {
            blocks_destroyed.insert(position);
        }
    }
}

// Insert robots and blocks destroyed by this explosion to 'destroyed_players' and
// 'destroyed_blocks' sets.
void BombExploded::update_destroyed_robots_and_blocks(std::set<player_id_t> &destroyed_robots,
                                                      std::set<Position> &destroyed_blocks) {
    for (auto const & robot_destroyed : robots_destroyed) {
        destroyed_robots.insert(robot_destroyed);
    }
    for (auto const & block_destroyed : blocks_destroyed) {
        destroyed_blocks.insert(block_destroyed);
    }

};

BombExploded::BombExploded(bomb_id_t id, GameState &game_state,
                           std::set<player_id_t> &destroyed_robots,
                           std::set<Position> &destroyed_blocks) :
        Event(EventType::BombExploded), id(id) {
    calculate_explosion(game_state);
    calculate_robots_destroyed(game_state);
    calculate_blocks_destroyed(game_state);
    update_destroyed_robots_and_blocks(destroyed_robots, destroyed_blocks);
}

// Executes the BombExploded event and updates 'game_state' and 'destroyed_players' and
// 'destroyed_blocks' sets accordingly.
void BombExploded::execute(GameState &game_state, std::set<player_id_t> &destroyed_robots,
                           std::set<Position> &destroyed_blocks) {
    calculate_explosion(game_state);
    update_destroyed_robots_and_blocks(destroyed_robots, destroyed_blocks);
    // Remove the bomb.
    game_state.bombs.erase(id);
}

void BombExploded::insert_to_buffer([[maybe_unused]] Buffer &buffer) const {
    auto event_type = static_cast<message_id_t>(type);
    buffer.insert(event_type, EVENT_TYPE_SIZE);
    buffer.insert(id, BOMB_ID_SIZE);
    buffer.insert((list_length_t) robots_destroyed.size(), LIST_LENGTH_SIZE);
    for (auto const & robot_destroyed : robots_destroyed) {
        buffer.insert(robot_destroyed, PLAYER_ID_SIZE);
    }
    buffer.insert((list_length_t) blocks_destroyed.size(), LIST_LENGTH_SIZE);
    for (auto const & block_destroyed : blocks_destroyed) {
        block_destroyed.insert_to_buffer(buffer);
    }
}

// Gets a PlayerMoved event from buffer.
PlayerMoved::PlayerMoved(Buffer &buffer) {
    buffer.get(id, PLAYER_ID_SIZE);
    Position position_copy(buffer);
    position = position_copy;
}

// Executes the PlayerMoved event and updates 'game_state' and 'destroyed_players' and
// 'destroyed_blocks' sets accordingly.
void PlayerMoved::execute(GameState &game_state) {
    // Update the player's position.
    game_state.player_positions[id] = position;
}

void PlayerMoved::insert_to_buffer(Buffer &buffer) const {
    auto event_type = static_cast<message_id_t>(type);
    buffer.insert(event_type, EVENT_TYPE_SIZE);
    buffer.insert(id, PLAYER_ID_SIZE);
    position.insert_to_buffer(buffer);
}

// Gets a BlockPlaced event from buffer.
BlockPlaced::BlockPlaced(Buffer &buffer) {
    Position position_copy(buffer);
    position = position_copy;
}

// Executes the BlockPlaced event and updates 'game_state' and 'destroyed_players' and
// 'destroyed_blocks' sets accordingly.
void BlockPlaced::execute(GameState &game_state) {
    // Add a block.
    game_state.blocks.insert(position);
}

void BlockPlaced::insert_to_buffer(Buffer &buffer) const {
    auto event_type = static_cast<message_id_t>(type);
    buffer.insert(event_type, EVENT_TYPE_SIZE);
    position.insert_to_buffer(buffer);
}