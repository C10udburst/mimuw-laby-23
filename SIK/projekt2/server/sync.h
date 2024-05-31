#ifndef PROJEKT2_SERVER_SYNC_H
#define PROJEKT2_SERVER_SYNC_H

#include <atomic>
#include "client.h"

namespace server {
    struct Sync {
        std::atomic_int current_game = 0;
        std::atomic_int current_player = 0;
        std::atomic_int current_draw = 0;
        std::atomic_bool game_running = true;
        int pipefd[2][4][2]{};
        int main_sem[2]{}; // semaphore for waking up main thread

        void wait(const Client& client, int type) const;
    public:
        Sync();
        ~Sync();

        void sem_sleep(const Client& client) const;
        void sem_wake(int seat) const;
        void barrier(const Client& client) const;
        void sleep_main() const;
        void wake_main() const;
    };
}

#endif //PROJEKT2_SERVER_SYNC_H
