#include "ArgParser.h"
#include "GameState.h"

#include <vector>
#include <iostream>
#include <array>
#include <cstring>
#include <chrono>

using namespace std;

void handle_message(const array<byte, 10> &message, size_t message_size, vector<GameState> &games, chrono::system_clock::time_point timestamp) {
    if(message_size < 5) {
        handle_invalid_message(message, message_size);
    }
    MessageType message_type = static_cast<MessageType>(message[0]);
    switch (message_type) {
        case MSG_JOIN:
            handle_join_message(message, message_size, games, timestamp);
            break;
        case MSG_MOVE_1:
        case MSG_MOVE_2:
            handle_move_message(message, message_size, games, timestamp);
            break;
        case MSG_KEEP_ALIVE:
            handle_keep_alive_message(message, message_size, games, timestamp);
            break;
        case MSG_GIVE_UP:
            handle_give_up_message(message, message_size, games, timestamp);
            break;
        default:
            handle_invalid_message(message, message_size);
    }
    while
}
int run_server(sockaddr_in server_addr, const vector<byte> &start_board, int timeout){
    vector<GameState> games;

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        cerr << "Failed to create socket: " << strerror(errno) << endl;
        return false;
    }

    if (bind(socket_fd, (struct sockaddr *) &server_addr, (socklen_t) sizeof(server_addr)) < 0) {
        cerr << "Failed to bind socket: " << strerror(errno) << endl;
        return false;
    }

    while(true){
        array<byte, 10> receive_buffer;
        sockaddr_in sender_addr{};
        socklen_t sender_len = sizeof(sender_addr);

        ssize_t rcv_bytes = recvfrom(socket_fd, receive_buffer.data(), receive_buffer.size(), 0,
                                     (struct sockaddr *) &sender_addr, &sender_len);

        if (rcv_bytes < 0) {
            cerr << "Failed to receive message: " << strerror(errno) << endl;
            continue;
        }
        auto timestamp = chrono::system_clock::now();
        handle_message(receive_buffer,rcv_bytes, games, timestamp);
    }

}
int main(int argc, char *argv[])
{
    sockaddr_in server_addr;
    vector<byte> start_board;
    int timeout;
    if (!parse_server_arguments(argc, argv, server_addr, start_board, timeout))
    {
        return 1;
    }
    run_server(server_addr, start_board, timeout);
}