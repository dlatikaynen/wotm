#pragma once
#include <string>

struct ClientInfo {
    std::string browser;
    std::string deviceType;
};

ClientInfo detect_client_info();
