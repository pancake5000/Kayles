#pragma once

#include <string>
#include <cstdint>
#include <netinet/in.h>
#include <vector>
#include "Message.h"

int parse_client_arguments(int argc, char *argv[], sockaddr_in& server_address, Message &message, int &timeout);
int parse_server_arguments(int argc, char *argv[], sockaddr_in& server_address, std::vector<uint8_t> &start_board, uint32_t &max_pawn, uint32_t &timeout);

