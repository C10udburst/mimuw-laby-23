#include "client.h"
#include "const.h"

bool server::Client::offer(utils::Connection *conn) {
    std::lock_guard<std::mutex> lock(mutex);
    if (connection)
        return false;
    connection = conn;
    return true;
}

bool server::Client::taken() const {
    return connection != nullptr;
}

void server::Client::release() {
    std::lock_guard<std::mutex> lock(mutex);
    delete connection;
    connection = nullptr;
}
