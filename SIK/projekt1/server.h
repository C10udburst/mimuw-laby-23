#ifndef PROJEKT1_SERVER_H
#define PROJEKT1_SERVER_H

#include <iostream>
#include "common.h"

namespace server {
    void handleConnection(const network::Connection& conn, char* buf);
    ppcb::types::Header* udprReceive(const network::Connection& conn, void* buf, uint64_t expected_pid);
    void send_rjt(const network::Connection &conn, void* buf);

    int tcpServer(uint16_t port);

    int udpServer(uint16_t port);
}

#endif //PROJEKT1_SERVER_H