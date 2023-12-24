#include "mimpi.h"
#include "mimpi_common.h"
#include "examples/mimpi_err.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

#define MAX_PATH_LENGTH 1024

#define PRINT_MIMPI_OK(expr, fd)                                                                    \
    do {                                                                                           \
        MIMPI_Retcode ret = expr;                                                                  \
        if (ret != MIMPI_SUCCESS) {                                                                \
            skprintf(                                                                              \
                fd,                                                                                \
                "MIMPI command failed: %s\n\tIn function %s() in %s line %d.\n\t Code: %i - %s\n", \
                #expr, __func__, __FILE__, __LINE__, ret, print_mimpi_error(ret)                   \
            );                                                                                     \
        }                                                                                          \
    } while(0)

void skprintf(int fd, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    write(fd, buf, n);
}

void print_open_descriptors(int fd) {
    const char* path = "/proc/self/fd";

    // Iterate over all symlinks in `path`.
    // They represent open file descriptors of our process.
    DIR* dr = opendir(path);
    if (dr == NULL)
        fatal("Could not open dir: %s", path);

    struct dirent* entry;
    while ((entry = readdir(dr)) != NULL) {
        if (entry->d_type != DT_LNK)
            continue;

        // Make a c-string with the full path of the entry.
        char subpath[MAX_PATH_LENGTH];
        int ret = snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
        if (ret < 0 || ret >= (int)sizeof(subpath))
            fatal("Error in snprintf");

        // Read what the symlink points to.
        char symlink_target[MAX_PATH_LENGTH];
        ssize_t ret2 = readlink(subpath, symlink_target, sizeof(symlink_target) - 1);
        ASSERT_SYS_OK(ret2);
        symlink_target[ret2] = '\0';

        // Skip an additional open descriptor to `path` that we have until closedir().
        if (strncmp(symlink_target, "/proc", 5) == 0)
            continue;
        
        skprintf(fd,  "Pid %d file descriptor %3s -> %s\n",
            getpid(), entry->d_name, symlink_target);
    }

    closedir(dr);
}

int parse_line(char* line, int len, int fd) {
   line[len] = '\0';
    if (strcmp(line, "exit\n") == 0) {
        return 1;
    } else if (strcmp(line, "rank\n") == 0) {
        skprintf(fd, "MPI rank: %d\n", MIMPI_World_rank());
    } else if (strcmp(line, "size\n") == 0) {
        skprintf(fd, "MPI size: %d\n", MIMPI_World_size());
    } else if (strcmp(line, "barrier\n") == 0) {
        skprintf(fd, "Barrier start\n");
        PRINT_MIMPI_OK(MIMPI_Barrier(), fd);
        skprintf(fd, "Barrier done\n");
    } else if (strcmp(line, "fd\n") == 0) {
        print_open_descriptors(fd);
    } else if (strncmp(line, "send ", 5) == 0) {
        int dest;
        int tag;
        int count;
        sscanf(line + 5, "%d %d %d", &dest, &tag, &count);
        skprintf(fd, "data (%dB):", count);
        char* data = malloc(count);
        read(fd, data, count);
        PRINT_MIMPI_OK(MIMPI_Send(data, count, dest, tag), fd);
        free(data);
    } else if (strncmp(line, "recv ", 5) == 0) {
        int src;
        int tag;
        int count;
        sscanf(line + 5, "%d %d %d", &src, &tag, &count);
        char* data = malloc(count);
        PRINT_MIMPI_OK(MIMPI_Recv(data, count, src, tag), fd);
        skprintf(fd, "data (%dB): %s\n", count, data);
        free(data);
    } else if (strncmp(line, "bcast ", 6) == 0) {
        int count;
        int root;
        sscanf(line + 6, "%d %d", &count, &root);
        skprintf(fd, "data (%dB):", count);
        char* data = malloc(count);
        read(fd, data, count);
        PRINT_MIMPI_OK(MIMPI_Bcast(data, count, root), fd);
        free(data);
    } else if (strncmp(line, "reduce ", 7) == 0) {
        int count;
        char op[7];
        int root;
        sscanf(line + 7, "%d %s %d", &count, op, &root);
        int op_real;
        if (strcmp(op, "MAX") == 0) {
            op_real = MIMPI_MAX;
        } else if (strcmp(op, "MIN") == 0) {
            op_real = MIMPI_MIN;
        } else if (strcmp(op, "SUM") == 0) {
            op_real = MIMPI_SUM;
        } else if (strcmp(op, "PROD") == 0) {
            op_real = MIMPI_PROD;
        } else {
            skprintf(fd, "Unknown op: %s\n", op);
            return 0;
        }
        skprintf(fd, "%d bytes:", count);
        char* data = malloc(count);
        read(fd, data, count);
        PRINT_MIMPI_OK(MIMPI_Reduce(data, data, count, op_real, root), fd);
        free(data);
    } else if (strcmp(line, "help\n") == 0) {
        skprintf(fd, "Commands:\n");
        skprintf(fd, "  exit\n");
        skprintf(fd, "  rank\n");
        skprintf(fd, "  size\n");
        skprintf(fd, "  barrier\n");
        skprintf(fd, "  fd\n");
        skprintf(fd, "  send <dest> <tag> <count>\n");
        skprintf(fd, "  recv <src> <tag> <count>\n");
        skprintf(fd, "  bcast <count> <root>\n");
        skprintf(fd, "  reduce <count> <op> <root>\n");
        skprintf(fd, "  help\n");
        skprintf(fd, "\n");
        skprintf(fd, "  tag = 0 -> MIMPI_TAG_ANY\n");
        skprintf(fd, "  op = MAX | MIN | SUM | PROD\n");
    } else {
        skprintf(fd, "Unknown command, type 'help' for help.\n");
    }
    return 0;
}

void start_konsole(const char* path) {
    sleep(1);
    char buf[1024];
    sprintf(buf, "konsole --new-tab -e bash -c \"socat - UNIX-CONNECT:%s; read\"", path);
    system(buf);
}

int main(int argc, char** argv) {
    MIMPI_Init(false);

    int rank = MIMPI_World_rank();

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_SYS_OK(fd);

    int res = mkdir("repl.sock.d", 0777);
    if (res == -1 && errno != EEXIST)
        ASSERT_SYS_OK(res);

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = ""
    };
    sprintf(addr.sun_path, "./repl.sock.d/%d", rank);
    
    
    ASSERT_SYS_OK(bind(fd, (struct sockaddr*)&addr, sizeof(addr)));
    printf("Listening on %s\n", addr.sun_path);

    if (argc > 1) {
        pthread_t thread;
        pthread_create(&thread, NULL, (void*(*)(void*))start_konsole, (void*)addr.sun_path);
    }

    ASSERT_SYS_OK(listen(fd, 1));
    int client_fd = accept(fd, NULL, NULL);
    ASSERT_SYS_OK(client_fd);
    printf("Accepted connection\n");

    while (1) {
        skprintf(client_fd, "%d> ", rank);
        char buf[1024];
        int n = read(client_fd, buf, sizeof(buf));
        ASSERT_SYS_OK(n);
        if (n == 0 || parse_line(buf, n, client_fd)) {
            close(client_fd);
            break;
        }
    }

    close(fd);
    unlink(addr.sun_path);
    MIMPI_Finalize();
}