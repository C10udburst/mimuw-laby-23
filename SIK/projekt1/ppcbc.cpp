#include <iostream>
#include <netinet/in.h>
#include "client.h"

std::vector<char> read_stdin() {
    // https://stackoverflow.com/questions/32954492/fastest-way-in-c-to-read-contents-of-stdin-into-a-string-or-vector
    std::vector<char> cin_str;
    // 64k buffer seems sufficient
    std::streamsize buffer_sz = 65536;
    std::vector<char> buffer(buffer_sz);
    cin_str.reserve(buffer_sz);

    auto rdbuf = std::cin.rdbuf();
    while (auto cnt_char = rdbuf->sgetn(buffer.data(), buffer_sz))
        cin_str.insert(cin_str.end(), buffer.data(), buffer.data() + cnt_char);

    return cin_str;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "ERROR: Invalid args. Usage:" << argv[0] << " <protocol> <ip> <port>" << std::endl;
        return 1;
    }

    try {
        uint16_t port = utils::read_port(argv[3]);
        std::string protocol = argv[1];
        std::string ip = argv[2];

        //auto data = read_stdin();
        auto data = std::vector<char>(1000);
        for (int i = 0; i < data.size(); i++) {
            data[i] = 'a' + (i % 26);
        }

        struct sockaddr_in addr = client::get_server_address(ip.c_str(), port);

        if (protocol == "tcp") {
            return client::tcpClient(addr, data);
        } else if (protocol == "udp") {
            return client::udpClient(addr, data, false);
        } else if (protocol == "udpr") {
            return client::udpClient(addr, data, true);
        } else {
            std::cerr << "ERROR: Invalid protocol. Use 'tcp', 'udp' or 'udpr'." << std::endl;
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}