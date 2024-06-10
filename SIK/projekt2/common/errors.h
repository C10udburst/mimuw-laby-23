#ifndef PROJEKT2_ERRORS_H
#define PROJEKT2_ERRORS_H

#include <stdexcept>
#include <utility>

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
        std::string line_start;
        explicit LineTooLongError(std::string line_start) : MainError("Line too long"), line_start(std::move(line_start)) {}
    };
}

#endif //PROJEKT2_ERRORS_H
