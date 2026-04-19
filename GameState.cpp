#include "GameState.h"
#include <arpa/inet.h>
#include <cstring>

bool get_pawn(const GameState &game_state, int idx)
{
    return game_state.pawn_row[idx / 8] & (1 << (idx % 8));
}
void remove_pawn(GameState &game_state, int idx)
{
    game_state.pawn_row[idx / 8] &= ~(1 << (idx % 8));
    game_state.num_pawns--;
}

void serialize_game_state(const GameState &game_state, array<byte, 46> &response)
{
    uint32_t net_game_id = htonl(game_state.game_id);
    uint32_t net_player_a_id = htonl(game_state.player_a_id);
    uint32_t net_player_b_id = htonl(game_state.player_b_id);

    memcpy(response.data() + 0, &net_game_id, sizeof(net_game_id));
    memcpy(response.data() + 4, &net_player_a_id, sizeof(net_player_a_id));
    memcpy(response.data() + 8, &net_player_b_id, sizeof(net_player_b_id));

    response[12] = static_cast<byte>(game_state.status);
    response[13] = static_cast<byte>(game_state.max_pawn);

    const size_t board_size = static_cast<size_t>(game_state.max_pawn / 8u + 1u);
    memcpy(response.data() + 14, game_state.pawn_row.data(), board_size);
}