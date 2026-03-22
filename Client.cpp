#include <iostream>
#include <cstdint>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdexcept>
#include <array>
#include "ArgParser.h"
#include "Message.h"
#include "GameState.h"
#include <cstring>
#include <vector>
#include <unistd.h>
#include <bitset>

using namespace std;

constexpr uint32_t max_response_length = (1 << 5) + 14;

array<byte, 10> serialize(const Message &message)
{
    array<byte, 10> result;

    result[0] = static_cast<byte>(message.type);

    uint32_t net_player_id = htonl(message.player_id);
    const byte *player_id_bytes = reinterpret_cast<const byte *>(&net_player_id);
    memcpy(&result[1], player_id_bytes, sizeof(net_player_id));

    uint32_t net_game_id = htonl(message.game_id);
    const byte *game_id_bytes = reinterpret_cast<const byte *>(&net_game_id);
    memcpy(&result[5], game_id_bytes, sizeof(net_game_id));

    result[9] = static_cast<byte>(message.pawn);

    return result;
}
void deserialize_and_print_response(const array<byte, max_response_length> &response, size_t response_size, const Message &message)
{
    if (response_size < 14)
    {
        cerr << "Wrong server response" << endl;
        return;
    }

    uint8_t status_code = static_cast<uint8_t>(response[12]);
    if (status_code > 4)
    {
        cerr << "Wrong server response" << endl;
        return;
    }

    if (response_size < 15)
    {
        string prefix{"Wrong message, server cannot understand your: "};
        switch (static_cast<uint8_t>(response[13]))
        {
        case 0:
            cerr << prefix + "message type." << endl;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            cerr << prefix + "player id." << endl;
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            cerr << prefix + "game id." << endl;
            break;
        case 9:
            cerr << prefix + "pawn number" << endl;
            break;
        default:
            cerr << "Wrong server response." << endl;
        }
        return;
    }

    GameState state(0);
    uint32_t net_game_id;
    memcpy(&net_game_id, &response[0], sizeof(net_game_id));
    state.game_id = ntohl(net_game_id);

    uint32_t net_player_a_id;
    memcpy(&net_player_a_id, &response[4], sizeof(net_player_a_id));
    state.player_a_id = ntohl(net_player_a_id);

    uint32_t net_player_b_id;
    memcpy(&net_player_b_id, &response[8], sizeof(net_player_b_id));
    state.player_b_id = ntohl(net_player_b_id);

    state.status = static_cast<uint8_t>(response[12]);
    state.max_pawn = static_cast<uint8_t>(response[13]);

    state.pawn_row.resize(response_size - 14);
    memcpy(state.pawn_row.data(), &response[14], response_size - 14);

    if (state.pawn_row.size() != state.max_pawn / 8u + 1u)
    {
        cerr << "Wrong server response" << endl;
        return;
    }
    cout << "Game ID: " << state.game_id << endl;
    cout << "Player A ID: " << state.player_a_id << " " << (message.player_id == state.player_a_id ? " (you)" : "") << endl;
    cout << "Player B ID: " << state.player_b_id << " " << (message.player_id == state.player_b_id ? " (you)" : "") << endl;
    cout << "Game Status: ";
    switch (state.status)
    {
    case WAITING_FOR_OPPONENT:
        cout << "Waiting for opponent" << endl;
        break;
    case TURN_A:
        cout << "Player A's turn" << endl;
        break;
    case TURN_B:
        cout << "Player B's turn" << endl;
        break;
    case WIN_A:
        cout << "Player A wins" << endl;
        if (message.player_id == state.player_a_id)
        {
            cout << "Congratulations!" << endl;
        }
        break;
    case WIN_B:
        cout << "Player B wins" << endl;
        if (message.player_id == state.player_b_id)
        {
            cout << "Congratulations!" << endl;
        }
        break;
    default:
        break;
    }
    cout << "Board state:" << endl;
    int x = 0;
    while (x + 8 <= state.max_pawn)
    {
        bitset<8> bits(state.pawn_row[x / 8]);
        cout << bits << " ";
        x += 8;
    }
    if (x < state.max_pawn)
    {
        bitset<8> bits(state.pawn_row[x / 8]);
        while (x < state.max_pawn)
        {
            cout << bits[x % 8];
            x++;
        }
    }
    string eight_space = "        ";
    for (size_t i = 0; i < state.pawn_row.size(); ++i)
    {
        cout << i * 8 << eight_space;
    }
    cout << endl;
    return;
}
bool send_and_receive(const sockaddr_in &server_addr, const Message &message, int timeout)
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        cerr << "Failed to create socket: " << strerror(errno) << endl;
        return false;
    }

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        cerr << "Failed to set socket timeout: " << strerror(errno) << endl;
        return false;
    }

    array<byte, 10> message_bytes = serialize(message);
    size_t message_size = get_message_length(message.type);
    ssize_t sent_bytes = sendto(socket_fd, message_bytes.data(), message_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0)
    {
        cerr << "Failed to send message: " << strerror(errno) << endl;
        close(socket_fd);
        return false;
    }

    array<byte, max_response_length> receive_buffer;
    sockaddr_in sender_addr{};
    socklen_t sender_len = sizeof(sender_addr);

    ssize_t rcv_bytes = recvfrom(socket_fd, receive_buffer.data(), receive_buffer.size(), 0,
                                 (struct sockaddr *)&sender_addr, &sender_len);

    if (rcv_bytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            cerr << "Timeout waiting for server response." << endl;
        }
        else
        {
            cerr << "Failed to receive message: " << strerror(errno) << endl;
        }
        close(socket_fd);
        return false;
    }

    close(socket_fd);

    deserialize_and_print_response(receive_buffer, rcv_bytes, message);

    return true;
}
int main(int argc, char *argv[])
{
    sockaddr_in server_addr;
    Message message;
    int timeout;

    if (!parse_client_arguments(argc, argv, server_addr, message, timeout))
    {
        return 1;
    }

    if (!send_and_receive(server_addr, message, timeout))
    {
        return 1;
    }

    return 0;
}