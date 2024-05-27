#ifndef PROJEKT2_ERRORS_H
#define PROJEKT2_ERRORS_H

#include <stdexcept>

namespace errors {
    class MainError : public std::runtime_error {
    public:
        explicit MainError(const std::string &what) : std::runtime_error(what) {}
    };

    class ConnClosedError : public MainError {
    public:
        explicit ConnClosedError() : MainError("Connection closed") {}
    };

    class TimeoutError : public MainError {
    public:
        explicit TimeoutError() : MainError("A timeout occurred") {}
    };

    class LineTooLongError : public MainError {
    public:
        explicit LineTooLongError() : MainError("Line too long") {}
    };
}

#endif //PROJEKT2_ERRORS_H
