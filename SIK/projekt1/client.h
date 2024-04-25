#ifndef PROJEKT1_CLIENT_H
#define PROJEKT1_CLIENT_H

#include <vector>
#include "common.h"

namespace client {
    struct sockaddr_in get_server_address(char const *host, uint16_t port);

    // send dataPacket using UDP with retransmissions, wait for ACK. throw on failure
    void udprSend(const network::Connection& conn, const ppcb::types::DataPacket* dataPacket, void* buf);
    // send all data, protocol agnostic
    void sendData(const network::Connection& conn, const std::vector<char>& data, void* buf);

    // setups connection and sends data
    int tcpClient(struct sockaddr_in addr, std::vector<char>& data);
    int udpClient(struct sockaddr_in addr, std::vector<char>& data, bool retransmit);

    uint64_t gen_session_id();
}

#endif //PROJEKT1_CLIENT_H