#ifndef __SERVER_H__

#define __SERVER_H__

#define _XOPEN_SOURCE 600
#include "builder.h"
#include "ev.h"
#include "ht.h"
#include "vec.h"
#include "vstr.h"
#include <stdint.h>
#include <time.h>

typedef struct {
    ht ht;
    vec* vec;
} lexidb;

typedef enum {
    None,
    Info,
    Debug,
    Verbose,
} log_level;

typedef struct {
    pid_t pid;
    int sfd;
    log_level log_level;
    uint16_t flags;
    uint16_t port;
    vstr addr;
    lexidb db;
    ev* ev;
    vec* clients;
    uint64_t cmds_processed;
} server;

typedef struct {
    int fd;
    uint32_t addr;
    uint16_t port;
    uint16_t flags;
    uint8_t* read_buf;
    size_t read_pos;
    size_t read_cap;
    uint8_t* write_buf;
    size_t write_size;
    builder builder;
    struct timespec time_connected;
} client;

typedef client* client_ptr;

int server_run(int argc, char* argv[]);

#endif /* __SERVER_H__ */
