#pragma once 
#include <cstdint>

enum MessageType : std::uint8_t
{
    MSG_JOIN = 0,
    MSG_MOVE_1 = 1,
    MSG_MOVE_2 = 2,
    MSG_KEEP_ALIVE = 3,
    MSG_GIVE_UP = 4
};

constexpr std::size_t get_message_length(MessageType type)
{
    switch (type)
    {
        case MSG_JOIN:
            return 5; // type (1) + player_id (4)
        case MSG_KEEP_ALIVE:
        case MSG_GIVE_UP:
            return 9; // type (1) + player_id (4) + game_id (4)
        case MSG_MOVE_1:
        case MSG_MOVE_2:
            return 10; // type (1) + player_id (4) + game_id (4) + pawn (1)
        default:
            return 0;
    }
}

struct Message{
    MessageType type;
    uint32_t player_id;
    uint32_t game_id;
    uint8_t pawn;
};