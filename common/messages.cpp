#include "messages.h"

#include <utility>

// Gets an InputMessage message from buffer.
InputMessage::InputMessage(Buffer &buffer) {
    size_t received_length = buffer.get_message_length();
    if (received_length == 0) {
        ignore = true;
        return;
    }

    message_id_t message_id;
    buffer.get(message_id, MESSAGE_ID_SIZE);
    type = static_cast<InputMessageType>(message_id);

    switch (type) {
        case InputMessageType::PlaceBomb:
            ignore = (received_length != PLACE_BOMB_MESSAGE_LENGTH);
            break;
        case InputMessageType::PlaceBlock:
            ignore = (received_length != PLACE_BLOCK_MESSAGE_LENGTH);
            break;
        case InputMessageType::Move:
            if (received_length != MOVE_MESSAGE_LENGTH) {
                ignore = true;
                return;
            }

            buffer.get(direction, DIRECTION_SIZE);
            switch (static_cast<Direction>(message_id)) {
                case Direction::Up:
                case Direction::Right:
                case Direction::Down:
                case Direction::Left:
                    break;
                default:
                    // Incorrect direction.
                    ignore = true;
                    break;
            }
            break;
        default:
            // Incorrect message id.
            ignore = true;
            break;
    }
}

// Creates an answer for the InputMessage message.
ClientMessage::ClientMessage(InputMessage input_message, GameState &game_state,
                             std::string name) {
    GameStateType current_game_state_type;
    {
        // Checking the current state of the game under the mutex.
        std::lock_guard<std::mutex> lk(game_state.mutex);
        current_game_state_type = game_state.type;
    }

    if (current_game_state_type == GameStateType::Lobby) {
        // State of the game is Lobby.
        type = ClientMessageType::Join;
        player_name = std::move(name);
    } else {
        // State of the game is Game.
        switch (input_message.get_type()) {
            case InputMessageType::PlaceBomb:
                type = ClientMessageType::PlaceBomb;
                break;
            case InputMessageType::PlaceBlock:
                type = ClientMessageType::PlaceBlock;
                break;
            case InputMessageType::Move:
                type = ClientMessageType::Move;
                direction = input_message.get_direction();
                break;
            default:
                break;
        }
    }
}

// Gets a ClientMessage message from buffer and updates 'game_state' accordingly.
ClientMessage::ClientMessage(Buffer &buffer) {//, GameState &game_state) {
    message_id_t message_id;
    buffer.get(message_id, MESSAGE_ID_SIZE);
    type = static_cast<ClientMessageType>(message_id);

    switch (type) {
        case ClientMessageType::Join: {
            buffer.get(player_name);
            break;
        }
        case ClientMessageType::PlaceBomb:
        case ClientMessageType::PlaceBlock:
            break;
        case ClientMessageType::Move: {
            buffer.get(direction, DIRECTION_SIZE);
            switch (static_cast<Direction>(direction)) {
                case Direction::Up:
                case Direction::Right:
                case Direction::Down:
                case Direction::Left:
                    break;
                default:
                    correct = false;
                    break;
            }
            break;
        }
        default:
            correct = false;
            break;
    }
}

// Inserts a ClientMessage message into buffer.
void ClientMessage::insert_to_buffer(Buffer &buffer) const {
    auto message_id = static_cast<message_id_t>(type);
    buffer.insert(message_id, MESSAGE_ID_SIZE);

    switch (type) {
        case ClientMessageType::Join:
            buffer.insert(player_name);
            break;
        case ClientMessageType::Move:
            buffer.insert(direction, DIRECTION_SIZE);
            break;
        default:
            break;
    }
}

// Gets a ServerMessage message from buffer and updates 'game_state' accordingly.
ServerMessage::ServerMessage(Buffer &buffer, GameState &game_state) {
    message_id_t message_id;
    buffer.get(message_id, MESSAGE_ID_SIZE);
    type = static_cast<ServerMessageType>(message_id);

    switch (type) {
        case ServerMessageType::Hello: {
            buffer.get(game_state.server_name);
            buffer.get(game_state.players_count, PLAYERS_COUNT_SIZE);
            buffer.get(game_state.size_x, COORDINATE_SIZE);
            buffer.get(game_state.size_y, COORDINATE_SIZE);
            buffer.get(game_state.game_length, GAME_LENGTH_SIZE);
            buffer.get(game_state.explosion_radius, EXPLOSION_RADIUS_SIZE);
            buffer.get(game_state.bomb_timer, BOMB_TIMER_SIZE);
            break;
        }
        case ServerMessageType::AcceptedPlayer: {
            player_id_t player_id;
            buffer.get(player_id, PLAYER_ID_SIZE);
            Player player(buffer);

            // Insert player to players and scores.
            game_state.players[player_id] = player;
            game_state.scores[player_id] = 0;
            break;
        }
        case ServerMessageType::GameStarted: {
            list_length_t players_size;
            buffer.get(players_size, LIST_LENGTH_SIZE);
            for (list_length_t i = 0; i < players_size; i++) {
                player_id_t player_id;
                buffer.get(player_id, PLAYER_ID_SIZE);
                Player player(buffer);

                // Insert players to players and scores.
                game_state.players[player_id] = player;
                game_state.scores[player_id] = 0;
            }

            {
                // Change state of the game to Game.
                std::lock_guard<std::mutex> lk(game_state.mutex);
                game_state.type = GameStateType::Game;
            }
            send_to_gui = false;
            break;
        }
        case ServerMessageType::Turn: {
            // Decrease all bombs' timers.
            for (auto & [bomb_id, bomb]: game_state.bombs) {
                bomb.decrease_timer();
            }
            // Clear explosions set.
            game_state.explosions.clear();

            buffer.get(game_state.turn, TURN_SIZE);

            list_length_t events_size;
            buffer.get(events_size, LIST_LENGTH_SIZE);
            for (list_length_t i = 0; i < events_size; i++) {
                event_type_t event_type;
                buffer.get(event_type, EVENT_TYPE_SIZE);
                switch (static_cast<EventType>(event_type)) {
                    case EventType::BombPlaced: {
                        BombPlaced bomb_placed(buffer);
                        bomb_placed.execute(game_state);
                        break;
                    }
                    case EventType::BombExploded: {
                        BombExploded bomb_exploded(buffer);
                        bomb_exploded.execute(game_state, robots_destroyed, blocks_destroyed);
                        break;
                    }
                    case EventType::PlayerMoved: {
                        PlayerMoved player_moved(buffer);
                        player_moved.execute(game_state);
                        break;
                    }
                    case EventType::BlockPlaced: {
                        BlockPlaced block_placed(buffer);
                        block_placed.execute(game_state);
                        break;
                    }
                    default:
                        break;
                }
            }

            // Give a point to every player who died in this turn.
            for (auto const & destroyed_player: robots_destroyed) {
                game_state.scores[destroyed_player]++;
            }

            // Remove blocks destroyed in this turn.
            for (auto const & destroyed_block: blocks_destroyed) {
                game_state.blocks.erase(destroyed_block);
            }

            break;
        }
        case ServerMessageType::GameEnded: {
            list_length_t scores_size;
            buffer.get(scores_size, LIST_LENGTH_SIZE);
            for (list_length_t i = 0; i < scores_size; i++) {
                player_id_t player_id;
                buffer.get(player_id, PLAYER_ID_SIZE);
                score_t score;
                buffer.get(score, SCORE_SIZE);

                // Update players' scores.
                game_state.scores[player_id] = score;
            }

            // Clear state of the game.
            game_state.players.clear();
            game_state.player_positions.clear();
            game_state.blocks.clear();
            game_state.bombs.clear();
            game_state.explosions.clear();
            game_state.scores.clear();

            {
                // Change state of the game to Lobby.
                std::lock_guard<std::mutex> lk(game_state.mutex);
                game_state.type = GameStateType::Lobby;
            }
            break;
        }
        default:
            // Incorrect message id.
            send_to_gui = false;
            throw std::invalid_argument("Incorrect message from server");
            break;
    }
}

// Inserts a ServerMessage message into buffer.
void ServerMessage::insert_to_buffer(Buffer &buffer, GameState &game_state) const {
    auto message_id = static_cast<message_id_t>(type);
    buffer.insert(message_id, MESSAGE_ID_SIZE);

    switch (type) {
        case ServerMessageType::Hello:
            buffer.insert(game_state.server_name);
            buffer.insert(game_state.players_count, PLAYERS_COUNT_SIZE);
            buffer.insert(game_state.size_x, COORDINATE_SIZE);
            buffer.insert(game_state.size_y, COORDINATE_SIZE);
            buffer.insert(game_state.game_length, GAME_LENGTH_SIZE);
            buffer.insert(game_state.explosion_radius, EXPLOSION_RADIUS_SIZE);
            buffer.insert(game_state.bomb_timer, BOMB_TIMER_SIZE);
            break;
        case ServerMessageType::AcceptedPlayer:
            buffer.insert(accepted_player_id, PLAYER_ID_SIZE);
            accepted_player.insert_to_buffer(buffer);
            break;
        case ServerMessageType::GameStarted:
            buffer.insert((list_length_t) players.size(), LIST_LENGTH_SIZE);
            for (auto const & [player_id, player]: players) {
                buffer.insert(player_id, PLAYER_ID_SIZE);
                player.insert_to_buffer(buffer);
            }
            break;
        case ServerMessageType::Turn:
            buffer.insert(turn, TURN_SIZE);
            buffer.insert((list_length_t) events.size(), LIST_LENGTH_SIZE);
            for (auto const & event: events) {
                event->insert_to_buffer(buffer);
            }
            break;
        case ServerMessageType::GameEnded:
            buffer.insert((list_length_t) scores.size(), LIST_LENGTH_SIZE);
            for (auto const & [player_id, score]: scores) {
                buffer.insert(player_id, PLAYER_ID_SIZE);
                buffer.insert(score, SCORE_SIZE);
            }
            break;
    }
}

// Creates an answer for the ServerMessage message.
DrawMessage::DrawMessage(ServerMessage server_message, [[maybe_unused]] GameState &game_state) {
    switch (server_message.get_type()) {
        case ServerMessageType::Hello:
        case ServerMessageType::AcceptedPlayer:
        case ServerMessageType::GameEnded:
            type = DrawMessageType::Lobby;
            break;
        case ServerMessageType::Turn:
            type = DrawMessageType::Game;
            break;
        default:
            break;
    }
}

// Inserts a DrawMessage message into buffer.
void DrawMessage::insert_to_buffer(Buffer &buffer, GameState &game_state) const {
    auto message_id = static_cast<message_id_t>(type);
    buffer.insert(message_id, MESSAGE_ID_SIZE);

    switch (type) {
        case DrawMessageType::Lobby:
            buffer.insert(game_state.server_name);
            buffer.insert(game_state.players_count, PLAYERS_COUNT_SIZE);
            buffer.insert(game_state.size_x, COORDINATE_SIZE);
            buffer.insert(game_state.size_y, COORDINATE_SIZE);
            buffer.insert(game_state.game_length, GAME_LENGTH_SIZE);
            buffer.insert(game_state.explosion_radius, EXPLOSION_RADIUS_SIZE);
            buffer.insert(game_state.bomb_timer, BOMB_TIMER_SIZE);

            buffer.insert((list_length_t) game_state.players.size(), LIST_LENGTH_SIZE);
            for (auto const & [player_id, player]: game_state.players) {
                buffer.insert(player_id, PLAYER_ID_SIZE);
                player.insert_to_buffer(buffer);
            }
            break;
        case DrawMessageType::Game:
            buffer.insert(game_state.server_name);
            buffer.insert(game_state.size_x, COORDINATE_SIZE);
            buffer.insert(game_state.size_y, COORDINATE_SIZE);
            buffer.insert(game_state.game_length, GAME_LENGTH_SIZE);
            buffer.insert(game_state.turn, TURN_SIZE);

            buffer.insert((list_length_t) game_state.players.size(), LIST_LENGTH_SIZE);
            for (auto const & [player_id, player]: game_state.players) {
                buffer.insert(player_id, PLAYER_ID_SIZE);
                player.insert_to_buffer(buffer);
            }

            buffer.insert((list_length_t) game_state.player_positions.size(), LIST_LENGTH_SIZE);
            for (auto const & [player_id, position]: game_state.player_positions) {
                buffer.insert(player_id, PLAYER_ID_SIZE);
                position.insert_to_buffer(buffer);
            }

            buffer.insert((list_length_t) game_state.blocks.size(), LIST_LENGTH_SIZE);
            for (auto const & block: game_state.blocks) {
                block.insert_to_buffer(buffer);
            }

            buffer.insert((list_length_t) game_state.bombs.size(), LIST_LENGTH_SIZE);
            for (auto const & bomb: game_state.bombs) {
                bomb.second.insert_to_buffer(buffer);
            }

            buffer.insert((list_length_t) game_state.explosions.size(), LIST_LENGTH_SIZE);
            for (auto const & explosion: game_state.explosions) {
                explosion.insert_to_buffer(buffer);
            }

            buffer.insert((list_length_t) game_state.scores.size(), LIST_LENGTH_SIZE);
            for (auto const & [player_id, score]: game_state.scores) {
                buffer.insert(player_id, PLAYER_ID_SIZE);
                buffer.insert(score, SCORE_SIZE);
            }
            break;
        default:
            break;
    }
}