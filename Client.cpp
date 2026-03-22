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

using namespace std;

constexpr uint32_t response_length = (1<<5) + 14;

array<byte, 10> serialize(const Message &message)
{
    array<byte, 10> result;
    
    result[0] = static_cast<byte>(message.type);
    
    uint32_t net_player_id = htonl(message.player_id);
    const byte* player_id_bytes = reinterpret_cast<const byte*>(&net_player_id);
    memcpy(&result[1], player_id_bytes, sizeof(net_player_id));
    
    uint32_t net_game_id = htonl(message.game_id);
    const byte* game_id_bytes = reinterpret_cast<const byte*>(&net_game_id);
    memcpy(&result[5], game_id_bytes, sizeof(net_game_id));

    result[9] = static_cast<byte>(message.pawn);
    
    return result;
}
int deserialize_and_print_response(const array<byte, response_length> &response)
{
    if (response.empty())
    {
        cerr << "Received empty response from server." << endl;
        return 1;
    }
    
    uint8_t status_code = static_cast<uint8_t>(response[0]);
    cout << "Server response status code: " << static_cast<int>(status_code) << endl;

    //TODO
    
    return 0;
}
bool send_and_receive(const sockaddr_in &server_addr, const Message &message, int timeout)
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        cerr << "Failed to create socket: " << strerror(errno) << endl;
        return 1;
    }

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "Failed to set socket timeout: " << strerror(errno) << endl;
        return 1;
    }

    array<byte, 10> message_bytes = serialize(message);
    size_t message_size = get_message_length(message.type);
    ssize_t sent_bytes = sendto(socket_fd, message_bytes.data(), message_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0) {
        cerr << "Failed to send message: " << strerror(errno) << endl;
        close(socket_fd);
        return false;
    }

    array<byte, response_length> receive_buffer;
    sockaddr_in sender_addr{};
    socklen_t sender_len = sizeof(sender_addr);

    ssize_t rcv_bytes = recvfrom(socket_fd, receive_buffer.data(), receive_buffer.size(), 0, 
                                 (struct sockaddr *)&sender_addr, &sender_len);

    if (rcv_bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            cerr << "Timeout waiting for server response." << endl;
        } else {
            cerr << "Failed to receive message: " << strerror(errno) << endl;
        }
        close(socket_fd);
        return false;
    }

    close(socket_fd);
    
    deserialize_and_print_response(receive_buffer);
    
    return true;
}
int main(int argc, char *argv[])
{
    sockaddr_in server_addr;
    Message message;
    int timeout;

    if (!parse_arguments(argc, argv, server_addr, message, timeout))
    {
        return 1;
    }

    

    if(!send_and_receive(server_addr, message, timeout))
    {
        return 1;
    }

    return 0;
}