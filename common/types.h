#ifndef TYPES_H
#define TYPES_H

// Types for parsing input parameters.
using port_t = uint16_t;
using port_parsing_t = int32_t;
using bomb_timer_parsing_t = int32_t;
using players_count_parsing_t = int16_t;
using turn_duration_parsing_t = std::string;
using explosion_radius_parsing_t = int32_t;
using initial_blocks_parsing_t = int32_t;
using game_length_parsing_t = int32_t;
using seed_parsing_t = int64_t;
using coordinate_parsing_t = int32_t;

constexpr players_count_parsing_t PLAYERS_COUNT_MAX = UINT8_MAX;

// Types for serialization and deserialization.
using string_length_t = uint8_t;
using list_length_t = uint32_t;
using message_id_t = uint8_t;
using direction_t = uint8_t;
using event_type_t = uint8_t;

constexpr size_t STRING_LENGTH_SIZE = 1;
constexpr size_t LIST_LENGTH_SIZE = 4;
constexpr size_t MESSAGE_ID_SIZE = 1;
constexpr size_t DIRECTION_SIZE = 1;
constexpr size_t EVENT_TYPE_SIZE = 1;

// Types for game.
using players_count_t = uint8_t;
using game_length_t = uint16_t;
using explosion_radius_t = uint16_t;
using bomb_timer_t = uint16_t;
using bomb_id_t = uint32_t;
using player_id_t = uint8_t;
using score_t = uint32_t;
using turn_t = uint16_t;
using coordinate_t = uint16_t;
using turn_duration_t = uint64_t;
using initial_blocks_t = uint16_t;
using seed_t = uint32_t;
using client_id_t = uint32_t;

constexpr size_t PLAYERS_COUNT_SIZE = 1;
constexpr size_t GAME_LENGTH_SIZE = 2;
constexpr size_t EXPLOSION_RADIUS_SIZE = 2;
constexpr size_t BOMB_TIMER_SIZE = 2;
constexpr size_t BOMB_ID_SIZE = 4;
constexpr size_t PLAYER_ID_SIZE = 1;
constexpr size_t SCORE_SIZE = 4;
constexpr size_t TURN_SIZE = 2;
constexpr size_t COORDINATE_SIZE = 2;
[[maybe_unused]] constexpr size_t TURN_DURATION_SIZE = 8;
[[maybe_unused]] constexpr size_t INITIAL_BLOCKS_SIZE = 2;
[[maybe_unused]] constexpr size_t SEED_SIZE = 4;

// Sizes of correct messages from GUI.
constexpr size_t PLACE_BOMB_MESSAGE_LENGTH = MESSAGE_ID_SIZE;
constexpr size_t PLACE_BLOCK_MESSAGE_LENGTH = MESSAGE_ID_SIZE;
constexpr size_t MOVE_MESSAGE_LENGTH = MESSAGE_ID_SIZE + DIRECTION_SIZE;

#endif //TYPES_H
