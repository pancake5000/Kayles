#include "ArgParser.h"
#include "GameState.h"

#include <vector>
#include <iostream>
#include <array>
#include <cstring>
#include <chrono>
#include <map>

using namespace std;
const int MAX_RESPONSE_MESSAGE_SLICE = 12;
const int INVALID_MESSAGE_RESPONSE_SIZE = 14;
typedef uint32_t GameID;

void add_to_last_move_time(map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time, chrono::steady_clock::time_point timestamp, GameID game_id, uint32_t player_id)
{
    while (last_move_time.find(timestamp) != last_move_time.end())
    {
        timestamp += chrono::nanoseconds(1);
    }
    last_move_time.insert({timestamp, {game_id, player_id}});
}
void adjust_timestamps_a(map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time, GameState &game_state, chrono::steady_clock::time_point timestamp)
{
    last_move_time.erase(game_state.last_move_A);
    game_state.last_move_A = timestamp;
    add_to_last_move_time(last_move_time, timestamp, game_state.game_id, game_state.player_a_id);
    last_move_time.insert({game_state.last_move_A, {game_state.game_id, game_state.player_a_id}});
}
void adjust_timestamps_b(map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time, GameState &game_state, chrono::steady_clock::time_point timestamp)
{
    last_move_time.erase(game_state.last_move_B);
    game_state.last_move_B = timestamp;
    add_to_last_move_time(last_move_time, timestamp, game_state.game_id, game_state.player_b_id);
    last_move_time.insert({game_state.last_move_B, {game_state.game_id, game_state.player_b_id}});
}
void deserialize_message(const array<byte, 12> &message_string, Message &message)
{
    message.type = static_cast<MessageType>(message_string[0]);

    if (message.type == MSG_JOIN)
    {
        return;
    }
    message.player_id = ntohl(static_cast<uint32_t>(message_string[1]));

    message.game_id = ntohl(static_cast<uint32_t>(message_string[5]));
    if (message.type == MSG_KEEP_ALIVE || message.type == MSG_GIVE_UP)
    {
        return;
    }
    message.pawn = static_cast<uint8_t>(message_string[9]);
}
void handle_invalid_message(const array<byte, 12> &message, int message_size, array<byte, 46> &response, bool &is_valid, int error_index)
{
    response[12] = static_cast<byte>(255);
    response[13] = static_cast<byte>(error_index);
    memcpy(response.data(), message.data(), min(message_size, MAX_RESPONSE_MESSAGE_SLICE));
    is_valid = false;
}
void handle_join_message(const Message &message, array<byte, 46> &response, map<GameID, GameState> &games, const GameState &start_state, chrono::steady_clock::time_point timestamp, map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time)
{
    static uint32_t last_game_id = 1;
    if (games.empty() || games[last_game_id].status != WAITING_FOR_OPPONENT)
    {
        GameState new_game;
        new_game.pawn_row = start_state.pawn_row;
        new_game.max_pawn = start_state.max_pawn;
        new_game.num_pawns = start_state.num_pawns;
        new_game.player_a_id = message.player_id;
        new_game.game_id = ++last_game_id;
        new_game.status = WAITING_FOR_OPPONENT;

        adjust_timestamps_a(last_move_time, games[new_game.game_id], timestamp);

        games[new_game.game_id] = new_game;
        serialize_game_state(games[new_game.game_id], response);
    }
    else
    {
        games[last_game_id].player_b_id = message.player_id;
        games[last_game_id].status = TURN_B;
        adjust_timestamps_b(last_move_time, games[last_game_id], timestamp);
        serialize_game_state(games[last_game_id], response);
    }
}
void handle_move_message(const Message &message, const array<byte, 12> &message_string, int message_size, array<byte, 46> &response, bool &is_valid, map<GameID, GameState> &games, chrono::steady_clock::time_point timestamp, map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time)
{

    if (games.find(message.game_id) == games.end())
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 5);
        return;
    }
    GameState &game_state = games[message.game_id];
    if ((game_state.player_a_id != message.player_id && game_state.player_b_id != message.player_id))
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 1);
        return;
    }
    if ((game_state.status == TURN_A && game_state.player_a_id != message.player_id) || (game_state.status == TURN_B && game_state.player_b_id != message.player_id) || !get_pawn(game_state, message.pawn) || (message.type == MSG_MOVE_2 && (message.pawn == game_state.max_pawn || !get_pawn(game_state, message.pawn + 1))) || game_state.status == WAITING_FOR_OPPONENT || game_state.status == WIN_A || game_state.status == WIN_B)
    {
        serialize_game_state(game_state, response);
    }
    else
    {
        remove_pawn(game_state, message.pawn);
        if (message.type == MSG_MOVE_2)
        {
            remove_pawn(game_state, message.pawn + 1);
        }
        if (game_state.num_pawns == 0)
        {
            if (game_state.status == TURN_A)
            {
                game_state.status = WIN_A;
            }
            else
            {
                game_state.status = WIN_B;
            }
        }
        else if (game_state.status == TURN_A)
        {

            adjust_timestamps_a(last_move_time, game_state, timestamp);
            game_state.status = TURN_B;
        }
        else
        {
            adjust_timestamps_b(last_move_time, game_state, timestamp);

            game_state.status = TURN_A;
        }
        serialize_game_state(game_state, response);
    }
}
void handle_keep_alive_message(const Message &message, const array<byte, 12> &message_string, int message_size, array<byte, 46> &response, bool &is_valid, map<GameID, GameState> &games, chrono::steady_clock::time_point timestamp, map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time)
{
    if (games.find(message.game_id) == games.end())
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 5);
        return;
    }
    GameState &game_state = games[message.game_id];
    if ((game_state.player_a_id != message.player_id && game_state.player_b_id != message.player_id))
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 1);
        return;
    }
    if (game_state.player_a_id == message.player_id)
    {
        adjust_timestamps_a(last_move_time, game_state, timestamp);
    }
    else
    {
        adjust_timestamps_b(last_move_time, game_state, timestamp);
    }
    serialize_game_state(game_state, response);
}
void handle_give_up_message(const Message &message, const array<byte, 12> &message_string, int message_size, array<byte, 46> &response, bool &is_valid, map<GameID, GameState> &games, chrono::steady_clock::time_point timestamp, map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time)
{
    if (games.find(message.game_id) == games.end())
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 5);
        return;
    }
    GameState &game_state = games[message.game_id];
    if ((game_state.player_a_id != message.player_id && game_state.player_b_id != message.player_id))
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 1);
        return;
    }
    if ((game_state.status == TURN_A && game_state.player_a_id != message.player_id) || (game_state.status == TURN_B && game_state.player_b_id != message.player_id) || game_state.status == WAITING_FOR_OPPONENT || game_state.status == WIN_A || game_state.status == WIN_B)
    {
        serialize_game_state(game_state, response);
    }
    else
    {
        if (game_state.status == TURN_A)
        {
            adjust_timestamps_a(last_move_time, game_state, timestamp);

            game_state.status = WIN_B;
        }
        else
        {
            adjust_timestamps_b(last_move_time, game_state, timestamp);

            game_state.status = WIN_A;
        }
        serialize_game_state(game_state, response);
    }
}
void handle_message(const array<byte, 12> &message_string, uint32_t message_size, array<byte, 46> &response, bool &is_valid, map<GameID, GameState> &games, const GameState &start_state, chrono::steady_clock::time_point timestamp, map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> &last_move_time)
{

    if (static_cast<uint8_t>(message_string[0]) > 4)
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 0);
        return;
    }
    MessageType message_type = static_cast<MessageType>(message_string[0]);
    if (message_size < get_message_length(message_type))
    {
        handle_invalid_message(message_string, message_size, response, is_valid, 0);
        return;
    }
    if (message_size > get_message_length(message_type))
    {
        handle_invalid_message(message_string, message_size, response, is_valid, get_message_length(message_type));
        return;
    }
    Message message;
    deserialize_message(message_string, message);
    // now we know message has valid type and size
    switch (message.type)
    {
    case MSG_JOIN:
        handle_join_message(message, response, games, start_state, timestamp, last_move_time);
        break;
    case MSG_MOVE_1:
    case MSG_MOVE_2:
        handle_move_message(message, message_string, message_size, response, is_valid, games, timestamp, last_move_time);
        break;
    case MSG_KEEP_ALIVE:
        handle_keep_alive_message(message, message_string, message_size, response, is_valid, games, timestamp, last_move_time);
        break;
    case MSG_GIVE_UP:
        handle_give_up_message(message, message_string, message_size, response, is_valid, games, timestamp, last_move_time);
        break;
    }
}
int run_server(sockaddr_in server_addr, const vector<uint8_t> &start_board, uint32_t max_pawn, uint32_t timeout)
{
    map<GameID, GameState> games;
    map<chrono::steady_clock::time_point, pair<GameID, uint32_t>> last_move_time;
    const int valid_response_size = 14 + max_pawn / 8 + 1;
    GameState start_state;
    start_state.pawn_row = start_board;
    start_state.max_pawn = max_pawn;
    start_state.num_pawns = max_pawn + 1;

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        cerr << "Failed to create socket: " << strerror(errno) << endl;
        return false;
    }

    if (bind(socket_fd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) < 0)
    {
        cerr << "Failed to bind socket: " << strerror(errno) << endl;
        return false;
    }

    while (true)
    {
        array<byte, 12> receive_buffer;
        sockaddr_in sender_addr{};
        socklen_t sender_len = sizeof(sender_addr);

        ssize_t rcv_bytes = recvfrom(socket_fd, receive_buffer.data(), receive_buffer.size(), 0,
                                     (struct sockaddr *)&sender_addr, &sender_len);

        if (rcv_bytes < 0)
        {
            cerr << "Failed to receive message: " << strerror(errno) << endl;
            continue;
        }
        auto timestamp = chrono::steady_clock::now();
        while (!last_move_time.empty() && chrono::duration_cast<chrono::seconds>(timestamp - last_move_time.begin()->first).count() >= timeout)
        {
            GameID game_id = last_move_time.begin()->second.first;
            uint32_t player_id = last_move_time.begin()->second.second;

            GameState &game_state = games[game_id];
            if (game_state.status != WIN_A && game_state.player_a_id == player_id)
            {
                game_state.status = WIN_B;
            }
            else if (game_state.status != WIN_B && game_state.player_b_id == player_id)
            {
                game_state.status = WIN_A;
            }
            if (game_state.player_a_id == player_id)
            {
                if (chrono::duration_cast<chrono::seconds>(timestamp - game_state.last_move_B).count() >= timeout)
                {
                    games.erase(game_id);
                }
            }
            if (game_state.player_b_id == player_id)
            {
                if (chrono::duration_cast<chrono::seconds>(timestamp - game_state.last_move_A).count() >= timeout)
                {
                    games.erase(game_id);
                }
            }
            last_move_time.erase(last_move_time.begin());
        }
        array<byte, 46> response;
        bool is_valid = true;
        handle_message(receive_buffer, static_cast<uint32_t>(rcv_bytes), response, is_valid, games, start_state, timestamp, last_move_time);
        int response_size = is_valid ? valid_response_size : INVALID_MESSAGE_RESPONSE_SIZE;
        ssize_t sent_bytes = sendto(socket_fd, response.data(), response_size, 0, (const struct sockaddr *)&sender_addr, sizeof(sender_addr));
        if (sent_bytes < 0)
        {
            cerr << "Failed to send message: " << strerror(errno) << endl;
            continue;
        }
    }
}
int main(int argc, char *argv[])
{
    sockaddr_in server_addr;
    vector<uint8_t> start_board;
    uint32_t timeout, max_pawn;
    if (!parse_server_arguments(argc, argv, server_addr, start_board, max_pawn, timeout))
    {
        return 1;
    }
    run_server(server_addr, start_board, max_pawn, timeout);
}