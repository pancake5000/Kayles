#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <array>
#include <cstddef>
using namespace std;

enum GameStatus : uint8_t
{
	WAITING_FOR_OPPONENT = 0,
	TURN_A = 1,
	TURN_B = 2,
	WIN_A = 3,
	WIN_B = 4,
};

struct GameState
{
	uint32_t game_id = 0;
	uint32_t player_a_id = 0;
	uint32_t player_b_id = 0;
	uint8_t status = WAITING_FOR_OPPONENT;
	uint8_t max_pawn = 0;
	uint32_t num_pawns = 0;
	vector<uint8_t> pawn_row;
	chrono::steady_clock::time_point last_move_A;
	chrono::steady_clock::time_point last_move_B;
	// explicit GameState(uint8_t max_pawn_value)
	// 	: max_pawn(max_pawn_value),
	// 	  pawn_row(static_cast<size_t>(max_pawn_value / 8) + 1, 0)
	// {
	// }
};

bool get_pawn(const GameState &game_state, int idx);
void remove_pawn(GameState &game_state, int idx);
void serialize_game_state(const GameState &game_state, array<byte, 46> &response);