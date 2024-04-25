#include "server.h"


int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "ERROR: Invalid args. Usage:" << argv[0] << " <protocol> <port>" << std::endl;
        return 1;
    }

    try {
        uint16_t port = utils::read_port(argv[2]);
        std::string protocol = argv[1];

        if (protocol == "tcp") {
            return server::tcpServer(port);
        } else if (protocol == "udp") {
            return server::udpServer(port);
        } else {
            std::cerr << "ERROR: Invalid protocol. Use 'tcp' or 'udp'." << std::endl;
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}