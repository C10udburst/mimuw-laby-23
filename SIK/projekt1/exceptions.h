#ifndef PROJEKT1_EXCEPTIONS_H
#define PROJEKT1_EXCEPTIONS_H

#include <stdexcept>

namespace network::exceptions {
    // we need a class to catch timeout for retransmission.
    class
    TimeoutException :
            public std::runtime_error {
    public:
        explicit TimeoutException() : std::runtime_error("Timeout reached") {}
    };

    class
    WrongPeerException :
            public std::runtime_error {
    public:
        explicit WrongPeerException() : std::runtime_error("Wrong peer address or session") {}
    };


    class
    UnexpectedPacketException :
            public std::runtime_error {
    public:
        uint8_t expected;
        uint8_t got;

        explicit UnexpectedPacketException(uint8_t expected, uint8_t got) : std::runtime_error(
                "Unexpected packet type. Expected: " + std::to_string(expected) + " Got: " + std::to_string(got)),
                                                                            expected(expected), got(got) {}
    };
} // namespace network::exceptions


#endif //PROJEKT1_EXCEPTIONS_H
