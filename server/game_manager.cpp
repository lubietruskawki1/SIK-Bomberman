#include "game_manager.h"

// Clears past messages queue and inserts a Hello message.
void GameManager::reset_past_messages() {
    std::lock_guard<std::mutex> lock(past_messages_mutex);
    past_messages = {};
    // Insert a Hello message.
    past_messages.push(std::make_shared<ServerMessage>(ServerMessageType::Hello));
}

Position GameManager::get_random_position() {
    auto x = (coordinate_t) (random() % game_state.size_x);
    auto y = (coordinate_t) (random() % game_state.size_y);
    return {x, y};
}

void GameManager::process_bombs(std::vector<std::shared_ptr<Event>> &events,
                                std::set<player_id_t> &current_turn_robots_destroyed) {
    std::set<bomb_id_t> current_turn_bombs_exploded;
    std::set<Position> current_turn_blocks_destroyed;
    for (auto & [bomb_id, bomb]: game_state.bombs) {
        // Decrease all bombs' timers.
        bomb.decrease_timer();
        if (bomb.get_timer() == 0) {
            // Explosion - insert a BombExploded event.
            events.push_back(std::make_shared<BombExploded>(bomb_id, game_state,
                                                            current_turn_robots_destroyed,
                                                            current_turn_blocks_destroyed));
            current_turn_bombs_exploded.insert(bomb_id);
            // Clear explosions set.
            game_state.explosions.clear();
        }
    }
    // Clear explosions set.
    game_state.explosions.clear();
    // Remove exploded bombs.
    for (auto & bomb_id: current_turn_bombs_exploded) {
        game_state.bombs.erase(bomb_id);
    }
    // Remove destroyed blocks.
    for (auto & position: current_turn_blocks_destroyed) {
        game_state.blocks.erase(position);
    }
}

void GameManager::process_place_bomb(std::vector<std::shared_ptr<Event>> &events,
                                     player_id_t player_id) {
    bomb_id_t bomb_id = bomb_id_generator.generate_id();
    Position position = game_state.player_positions[player_id];
    game_state.bombs[bomb_id] = Bomb(position, game_state.bomb_timer);
    // Insert a BombPlaced event.
    events.push_back(std::make_shared<BombPlaced>(bomb_id, position));
}

void GameManager::process_place_block(std::vector<std::shared_ptr<Event>> &events,
                                      player_id_t player_id) {
    Position position = game_state.player_positions[player_id];
    if (!game_state.blocks.contains(position)) {
        game_state.blocks.insert(position);
        // Insert a BlockPlaced event.
        events.push_back(std::make_shared<BlockPlaced>(position));
    }
}

void GameManager::process_move(std::vector<std::shared_ptr<Event>> &events, player_id_t player_id,
                               direction_t direction) {
    Position position = game_state.player_positions[player_id];
    coordinate_t x = position.get_x();
    coordinate_t y = position.get_y();
    Position new_position = position;
    switch (static_cast<Direction>(direction)) {
        case Direction::Up:
            if (y < game_state.size_y - 1) {
                new_position = Position(x, (coordinate_t) (y + 1));
            }
            break;
        case Direction::Right:
            if (x < game_state.size_x - 1) {
                new_position = Position((coordinate_t) (x + 1), y);
            }
            break;
        case Direction::Down:
            if (y > 0) {
                new_position = Position(x, (coordinate_t) (y - 1));
            }
            break;
        case Direction::Left:
            if (x > 0) {
                new_position = Position((coordinate_t) (x - 1), y);
            }
            break;
    }
    if (new_position != position && !game_state.blocks.contains(new_position)) {
        game_state.player_positions[player_id] = new_position;
        // Insert a PlayerMoved event.
        events.push_back(std::make_shared<PlayerMoved>(player_id, new_position));
    }
}

void GameManager::process_player_moves(std::vector<std::shared_ptr<Event>> &events,
                                       std::map<player_id_t, ClientMessage> current_turn_messages,
                                       const std::set<player_id_t> &current_turn_robots_destroyed) {
    for (auto & [player_id, player] : game_state.players) {
        if (!current_turn_robots_destroyed.contains(player_id)) {
            // Robot wasn't destroyed.
            if (current_turn_messages.contains(player_id)) {
                // Player made a move.
                ClientMessage client_message = current_turn_messages[player_id];
                switch (client_message.get_type()) {
                    case ClientMessageType::PlaceBomb: {
                        process_place_bomb(events, player_id);
                        break;
                    }
                    case ClientMessageType::PlaceBlock: {
                        process_place_block(events, player_id);
                        break;
                    }
                    case ClientMessageType::Move: {
                        direction_t direction = client_message.get_direction();
                        process_move(events, player_id, direction);
                        break;
                    }
                    default:
                        break;
                }
            }
        } else {
            // Robot was destroyed.
            game_state.scores[player_id]++;
            Position position = get_random_position();
            game_state.player_positions[player_id] = position;
            events.push_back(std::make_shared<PlayerMoved>(player_id, position));
        }
    }
}

void GameManager::add_player(const Player &player, client_id_t client_id) {
    player_id_t player_id;
    bool insertion_success = false;
    {
        std::lock_guard<std::mutex> lock(game_state.mutex);
        if (game_state.type == GameStateType::Lobby &&
            game_state.players_count != game_state.players.size() &&
            !client_to_player_id.contains(client_id)) {
            player_id = player_id_generator.generate_id();
            game_state.players[player_id] = player;
            game_state.scores[player_id] = 0;
            client_to_player_id[client_id] = player_id;
            insertion_success = true;
        }
    }
    if (insertion_success) {
        // Send AcceptedPlayer message.
        pending_messages.push(std::make_shared<ServerMessage>(
                ServerMessageType::AcceptedPlayer, player_id, player));
    }
}

void GameManager::start_game() {
    {
        std::lock_guard<std::mutex> lock(game_state.mutex);
        game_state.type = GameStateType::Game;
    }
    reset_past_messages();
    // Send GameStarted message.
    pending_messages.push(std::make_shared<ServerMessage>(ServerMessageType::GameStarted, game_state));
}

void GameManager::initialize_game_state() {
    game_state.turn = TURN_ZERO;
    std::vector<std::shared_ptr<Event>> events;

    // Place players' robots in random positions.
    for (auto & [player_id, player]: game_state.players) {
        Position position = get_random_position();
        game_state.player_positions[player_id] = position;
        // Insert a PlayerMoved event.
        events.push_back(std::make_shared<PlayerMoved>(player_id, position));
    }

    // Place blocks in random positions.
    for (initial_blocks_t i = 0; i < game_state.initial_blocks; i++) {
        Position position = get_random_position();
        if (!game_state.blocks.contains(position)) {
            game_state.blocks.insert(position);
            // Insert a BlockPlaced event.
            events.push_back(std::make_shared<BlockPlaced>(position));
        }
    }

    // Send Turn message.
    pending_messages.push(std::make_shared<ServerMessage>(ServerMessageType::Turn, TURN_ZERO, events));
}

void GameManager::run_turn(turn_t turn,
                           std::map<player_id_t, ClientMessage> current_turn_messages) {
    game_state.turn = turn;
    std::vector<std::shared_ptr<Event>> events;
    std::set<player_id_t> current_turn_robots_destroyed;

    process_bombs(events, current_turn_robots_destroyed);
    process_player_moves(events, std::move(current_turn_messages), current_turn_robots_destroyed);

    // Send Turn message.
    pending_messages.push(std::make_shared<ServerMessage>(ServerMessageType::Turn, turn, events));
}

void GameManager::end_game() {
    {
        std::lock_guard<std::mutex> lock(game_state.mutex);
        game_state.type = GameStateType::Lobby;
    }
    // Send GameEnded message.
    pending_messages.push(std::make_shared<ServerMessage>(ServerMessageType::GameEnded, game_state));
}

void GameManager::reset_game_state() {
    {
        std::lock_guard<std::mutex> lock(game_state.mutex);
        game_state.players.clear();
        game_state.player_positions.clear();
        game_state.blocks.clear();
        game_state.bombs.clear();
        game_state.explosions.clear();
        game_state.scores.clear();
        player_id_generator.reset();
        client_to_player_id.clear();
    }
    reset_past_messages();
    bomb_id_generator.reset();
}