#include "ArgParser.h"
#include <iostream>
#include <limits>
#include <netdb.h>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>

using namespace std;

constexpr uint32_t MAX_TIMEOUT = 99;
constexpr uint32_t MAX_PORT = 65535;

static bool is_valid_server_address(const string &address)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo *result = nullptr;
    const int status = getaddrinfo(address.c_str(), nullptr, &hints, &result);
    if (status != 0)
    {
        return false;
    }

    freeaddrinfo(result);
    return true;
}
bool get_server_address(const string &address, uint16_t port, sockaddr_in &server_addr)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo *result = nullptr;
    int status = getaddrinfo(address.c_str(), nullptr, &hints, &result);
    if (status != 0)
    {
        cerr << "Failed to resolve server address: " << gai_strerror(status) << endl;
        return false;
    }

    sockaddr_in server_addr{};
    if (result != nullptr)
    {
        server_addr = *reinterpret_cast<sockaddr_in *>(result->ai_addr);
        
        server_addr.sin_port = htons(port);
        
        freeaddrinfo(result);
    }

    return true;
}
bool try_parse_u32(const string &text, uint32_t &value)
{
    if (text.empty() || text[0] == '-')
    {
        return false;
    }

    try
    {
        size_t parsed_chars = 0;
        unsigned long parsed_value = stoul(text, &parsed_chars);
        if (parsed_chars != text.size())
        {
            return false;
        }
        if (parsed_value > numeric_limits<uint32_t>::max())
        {
            return false;
        }
        value = static_cast<uint32_t>(parsed_value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool parse_message(const string &text, Message &message)
{
    vector<string> chunks;
    size_t pos = 0;
    int count = 0;
    while (count < 4 && pos < text.size())
    {
        size_t next_pos = text.find('/', pos);
        if (next_pos == string::npos)
        {
            next_pos = text.size();
        }

        chunks.push_back(text.substr(pos, next_pos - pos));

        count++;
        pos = next_pos + 1;
    }
    if (pos != text.size() + 1)
    {
        cerr << "Too many fields in message: " << text << ". Expected up to 4 fields separated by '/'." << endl;
        return 1;
    }
    if (chunks.size() < 2)
    {
        return false;
    };
    uint32_t message_type_u32 = 0;
    if (!try_parse_u32(chunks[0], message_type_u32) || message_type_u32 > MSG_GIVE_UP)
    {
        cerr << "Invalid message type: " << chunks[0] << ". Expected an integer between 0 and 4." << endl;
        return false;
    }
    message.type = static_cast<MessageType>(message_type_u32);

    if (!try_parse_u32(chunks[1], message.player_id) || message.player_id < 1)
    {
        cerr << "Invalid player_id: " << chunks[1] << ". Expected a positive integer." << endl;
        return false;
    }

    if (message.type == MSG_JOIN)
    {
        return true;
    }
    if (chunks.size() < 3)
    {
        cerr << "Missing game_id for message type " << message.type << endl;
        return false;
    }
    if (!try_parse_u32(chunks[2], message.game_id))
    {
        cerr << "Invalid game_id: " << chunks[2] << ". Expected an unsigned integer." << endl;
        return false;
    }
    if (message.type == MSG_KEEP_ALIVE || message.type == MSG_GIVE_UP)
    {
        return true;
    }
    if (chunks.size() < 4)
    {
        cerr << "Missing pawn for message type " << message.type << endl;
        return false;
    }
    uint32_t pawn_u32 = 0;
    if (!try_parse_u32(chunks[3], pawn_u32) || pawn_u32 > 255)
    {
        cerr << "Invalid pawn: " << chunks[3] << ". Expected an integer between 0 and 255." << endl;
        return false;
    }
    message.pawn = static_cast<uint8_t>(pawn_u32);

    return 0;
}

int parse_arguments(int argc, char *argv[], sockaddr_in &server_addr, Message &message, int &timeout)
{
    if (argc != 9)
    {
        cerr << "Usage: " << argv[0] << " -a <server_address> -p <server_port> -m <message> -t <timeout>" << endl;
        return 1;
    }
    unordered_map<string, string> args;
    for (int i = 1; i < argc; i += 2)
    {
        args[argv[i]] = argv[i + 1];
    }

    if (!args.count("-a") || !args.count("-p") || !args.count("-m") || !args.count("-t"))
    {
        cerr << "Missing required arguments. Usage: " << argv[0] << " -a <server_address> -p <server_port> -m <message> -t <timeout>" << endl;
        return 1;
    }

    string server_address = args["-a"];
    if (!is_valid_server_address(server_address))
    {
        cerr << "Invalid server_address: " << server_address << ". Expected a valid IPv4 address or resolvable hostname." << endl;
        return 1;
    }

    uint32_t server_port_u32 = 0;
    if (!try_parse_u32(args["-p"], server_port_u32) || server_port_u32 < 1 || server_port_u32 > MAX_PORT)
    {
        cerr << "Invalid port: " << args["-p"] << ". Expected an unsigned integer in range 1..65535." << endl;
        return 1;
    }
    uint16_t server_port = static_cast<uint16_t>(server_port_u32);

    if (!get_server_address(server_address, server_port, server_addr))
    {
        return 1;
    }

    parse_message(args["-m"], message);

    uint32_t timeout_u32 = 0;
    if (!try_parse_u32(args["-t"], timeout_u32) || timeout_u32 < 1 || timeout_u32 > MAX_TIMEOUT)
    {
        cerr << "Invalid timeout: " << args["-t"] << ". Expected an integer between 1 and " << MAX_TIMEOUT << "." << endl;
        return 1;
    }

    return 0;
}
