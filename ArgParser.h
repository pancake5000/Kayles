#pragma once

#include <string>
#include <cstdint>
#include "Message.h"

int parse_arguments(int argc, char *argv[], sockaddr_in& server_address, Message &message, int &timeout);


