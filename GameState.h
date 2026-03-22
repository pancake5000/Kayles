#pragma once

#include <cstdint>
#include <vector>

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
	uint32_t player_a_id = 1;
	uint32_t player_b_id = 0;
	uint8_t status = WAITING_FOR_OPPONENT;
	uint8_t max_pawn = 0;
	vector<uint8_t> pawn_row;

	explicit GameState(uint8_t max_pawn_value)
		: max_pawn(max_pawn_value),
		  pawn_row(static_cast<size_t>(max_pawn_value / 8) + 1, 0)
	{
	}
};