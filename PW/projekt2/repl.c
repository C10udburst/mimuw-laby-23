#include "mimpi.h"
#include "mimpi_common.h"
#include "channel.h"
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
#include <sys/file.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>

#define MAX_PATH_LENGTH 1024

#define PRINT_MIMPI_OK(expr, fd)                                                                   \
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

#define PRINT_SYS_OK(expr, fd)                                                                       \
    do {                                                                                             \
        if ((expr) == -1)                                                                            \
            skprintf(                                                                                \
                fd,                                                                                  \
                "system command failed: %s\n\tIn function %s() in %s line %d.\n\tErrno: (%d; %s)",   \
                #expr, __func__, __FILE__, __LINE__, errno, strerror(errno)                          \
            );                                                                                       \
    } while(0)

#define if_cmd(name)                                        \
    else if (strncmp(line, name, sizeof(name) - 1) == 0)


void skprintf(int fd, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
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
        PRINT_SYS_OK(ret2, fd);
        symlink_target[ret2] = '\0';

        // Skip an additional open descriptor to `path` that we have until closedir().
        if (strncmp(symlink_target, "/proc", 5) == 0)
            continue;
        
        skprintf(fd,  "fd %3s -> %s\n",
            entry->d_name, symlink_target);
    }

    closedir(dr);
}

void parse_fd_flags(int flags, int printfd) {
    skprintf(printfd, "flags (0x%x): ", flags);
    if (flags & O_WRONLY)
        skprintf(printfd, "O_WRONLY ");
    if (flags & O_RDWR)
        skprintf(printfd, "O_RDWR ");
    if (flags & O_RDONLY)
        skprintf(printfd, "O_RDONLY ");
    if (flags & O_APPEND)
        skprintf(printfd, "O_APPEND ");
    if (flags & O_ASYNC)
        skprintf(printfd, "O_ASYNC ");
    if (flags & O_CLOEXEC)
        skprintf(printfd, "O_CLOEXEC ");
    if (flags & O_CREAT)
        skprintf(printfd, "O_CREAT ");
    if (flags & O_DIRECTORY)
        skprintf(printfd, "O_DIRECTORY ");
    if (flags & O_DSYNC)
        skprintf(printfd, "O_DSYNC ");
    if (flags & O_EXCL)
        skprintf(printfd, "O_EXCL ");
    if (flags & O_NOCTTY)   
        skprintf(printfd, "O_NOCTTY ");
    if (flags & O_NOFOLLOW)
        skprintf(printfd, "O_NOFOLLOW ");
    if (flags & O_NONBLOCK)
        skprintf(printfd, "O_NONBLOCK ");
    if (flags & O_RSYNC)
        skprintf(printfd, "O_RSYNC ");
    if (flags & O_SYNC)
        skprintf(printfd, "O_SYNC ");
    if (flags & O_TRUNC)
        skprintf(printfd, "O_TRUNC ");
    skprintf(printfd, "\n");
}

bool initialized = false;

extern char **environ;

char* read_bytes(int fd, int count) {
    char* data = malloc(count);
    memset(data, '*', count);
    skprintf(fd, "data (%dB):", count);
    read(fd, data, count);
    return data;
}

int parse_line(char* line, int len, int fd) {
    line[len] = '\0';
    if (strcmp(line, "\n") == 0)
       return 0;
    if_cmd("exit")
        return 1;
    if_cmd("rank")
        skprintf(fd, "MPI rank: %d\n", MIMPI_World_rank());
    if_cmd("init?")
        skprintf(fd, "MPI initialized: %d\n", initialized);
    if_cmd("size")
        skprintf(fd, "MPI size: %d\n", MIMPI_World_size());
    if_cmd("barrier") {
        skprintf(fd, "Barrier start\n");
        PRINT_MIMPI_OK(MIMPI_Barrier(), fd);
        skprintf(fd, "Barrier done\n");
    } if_cmd("fd")
        print_open_descriptors(fd);
    if_cmd("send ") {
        int dest;
        int tag;
        int count;
        sscanf(line + 5, "%d %d %d", &dest, &tag, &count);
        char* data = read_bytes(fd, count);
        PRINT_MIMPI_OK(MIMPI_Send(data, count, dest, tag), fd);
        free(data);
    } if_cmd("recv ") {
        int src;
        int tag;
        int count;
        sscanf(line + 5, "%d %d %d", &src, &tag, &count);
        char* data = malloc(count + 1);
        PRINT_MIMPI_OK(MIMPI_Recv(data, count, src, tag), fd);
        data[count] = '\0';
        skprintf(fd, "data (%dB): %s\n", count, data);
        free(data);
    } if_cmd("bcast ") {
        int count;
        int root;
        sscanf(line + 6, "%d %d", &count, &root);
        char* data = (MIMPI_World_rank() == root) ? read_bytes(fd, count) : malloc(count);
        PRINT_MIMPI_OK(MIMPI_Bcast(data, count, root), fd);
        if (MIMPI_World_rank() != root) {
            data[count] = '\0';
            skprintf(fd, "data (%dB): %s\n", count, data);
        }
        free(data);
    } if_cmd("reduce ") {
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
        char* in_data = read_bytes(fd, count);
        char* out_data = malloc(count + 1);
        PRINT_MIMPI_OK(MIMPI_Reduce(in_data, out_data, count, op_real, root), fd);
        if (MIMPI_World_rank() == root) {
            out_data[count] = '\0';
            skprintf(fd, "data (%dB): %s\n", count, out_data);   
        }
        free(in_data);
        free(out_data);
    } if_cmd("chrecv ") {
        int count;
        int myfd;
        sscanf(line + 7, "%d %d", &myfd, &count);
        char* data = malloc(count + 1);
        skprintf(fd, "reading %dB\n", count);
        PRINT_SYS_OK(chrecv(myfd, data, count), fd);
        skprintf(fd, "data (%dB): %s\n", count, data);
        free(data);
    } if_cmd("chsend ") {
        int count;
        int myfd;
        sscanf(line + 7, "%d %d", &myfd, &count);
        char* data = read_bytes(fd, count);
        skprintf(fd, "sending %dB\n", count);
        PRINT_SYS_OK(chsend(myfd, data, count), fd);
        free(data);
    } if_cmd("init") {
        MIMPI_Init(0);
        initialized = true;
    } if_cmd("init deadlock") {
        MIMPI_Init(1);
        initialized = true;
    } if_cmd("finalize") {
        MIMPI_Finalize();
        initialized = false;
    } if_cmd("fflags") {
        int myfd;
        sscanf(line + 7, "%d", &myfd);
        int flags = fcntl(myfd, F_GETFL);
        PRINT_SYS_OK(flags, flags);
        parse_fd_flags(flags, fd);
    } if_cmd("sdelay w ") {
        char* delay_str = malloc(20);
        sscanf(line + 9, "%s", delay_str);
        PRINT_SYS_OK(setenv("CHANNELS_WRITE_DELAY", delay_str, 1), fd);
    } if_cmd("sdelay r ") {
        char* delay_str = malloc(20);
        sscanf(line + 9, "%s", delay_str);
        PRINT_SYS_OK(setenv("CHANNELS_READ_DELAY", delay_str, 1), fd);
    } if_cmd("env") {
        char** env = environ;
        for (; *env; ++env)
            skprintf(fd, "%s\n", *env);
    } if_cmd("help") {
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
        skprintf(fd, "  chrecv <fd> <count>\n");
        skprintf(fd, "  chsend <fd> <count>\n");
        skprintf(fd, "  fflags <fd>\n");
        skprintf(fd, "  init [deadlock]\n");
        skprintf(fd, "  finalize\n");
        skprintf(fd, "  init?\n");
        skprintf(fd, "  env\n");
        skprintf(fd, "  sdelay <w|r> <delay>\n");
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

bool has(int argc, char** argv, char* v) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], v) == 0)
            return true;
    }
    return false;
}

int main(int argc, char** argv) {

    if (!has(argc, argv, "noinit")) {
        MIMPI_Init(has(argc, argv, "deadlock"));
        initialized = true;
    }

    char* rank_str = getenv("MIMPI_RANK");
    int rank = atoi(rank_str);

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

    pthread_t thread;
    if (has(argc, argv, "konsole")) {
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
    if (initialized)
        MIMPI_Finalize();

    if (has(argc, argv, "konsole")) {
        pthread_join(thread, NULL);
    }
}