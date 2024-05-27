#ifndef PROJEKT2_SERVER_SERVER_H
#define PROJEKT2_SERVER_SERVER_H

#include "client.h"
#include "gamefile.h"
#include "sync.h"

namespace server {
    struct Server {
        Client clients[4];
        Gamefile gamefile;
        Sync sync;

    public:
        void handle_new(utils::Connection *conn);
        void handle_game(server::Client &client);
    };


}


#endif //PROJEKT2_SERVER_SERVER_H
