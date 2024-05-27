#ifndef PROJEKT2_SERVER_CLIENT_H
#define PROJEKT2_SERVER_CLIENT_H

#include <mutex>
#include "../common/connection.h"

namespace server {

    struct Client {
        std::mutex mutex;
        utils::Connection *connection = nullptr;
        int seat{};
        int current_draw = 0;
        bool dirty = false;
        int scores[2] = {0, 0};

        ~Client() {
            delete connection;
        }

        bool offer(utils::Connection *conn);
        [[nodiscard]] bool taken() const;
        void release();
    };
}


#endif //PROJEKT2_SERVER_CLIENT_H
