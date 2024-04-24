#ifndef __SERVER_H__

#define __SERVER_H__

#include "cmd.h"
#include "ev.h"
#include "vec.h"
#include "vstr.h"
#include <signal.h>
#include <stdint.h>
#include <time.h>

#define BIND_ADDR_MAX 16

typedef struct lexi_server lexi_server;
typedef struct client client;
typedef struct connection connection;
typedef struct user user;

typedef enum {
    LL_Nothing = 0,
    LL_Info = 1,
    LL_Warning = 2,
    LL_Verbose = 3,
    LL_Debug = 4,
} log_level;

struct lexi_server {
    pid_t pid;              /* pid of the process */
    pthread_t thread_id;    /* main thread id */
    char* config_file;      /* absolute path to config file */
    char* executable;       /* absolute path to this executable */
    int argc;               /* the number of args passed to executable */
    char** argv;            /* the args passsed to executable */
    ev* ev;                 /* event loop */
    uint16_t port;          /* the tcp port */
    vstr bind_addr;         /* the address to bind to */
    vec* users;             /* list of configured users */
    vec* clients;           /* list of active clients */
    log_level loglevel;     /* the amount to log */
    char* logfile;          /* path to log to. if null, logs sent to stdout */
    int protected_mode;     /* are we in protected mode? */
    int tcp_backlog;        /* backlog passed to listen() */
    uint64_t num_databases; /* the number of databases */
};

struct client {
    uint64_t id;              /* autoincremented unique id */
    connection* conn;         /* connection */
    vstr name;                /* name */
    user* user;               /* user associated with this connection */
    time_t creation_time;     /* time this client was created */
    int authenticated;        /* is this client authenticated */
    size_t read_buf_pos;      /* position in buf */
    size_t read_buf_cap;      /* buf capacity */
    unsigned char* read_buf;  /* read buf */
    size_t write_pos;         /* position that we have written up to */
    size_t write_len;         /* the amount to write */
    unsigned char* write_buf; /* the buffer to write */
};

typedef union {
    category category;
    cmd_type cmd;
} rule;

#define USER_ON (1 << 0)
#define USER_NO_PASS (1 << 1)
#define USER_DEFAULT (1 << 2)

struct user {
    vstr username;   /* the username */
    uint32_t flags;  /* flags */
    vec* passwords;  /* vector of allowed passwords */
    vec* categories; /* vector of allowed categories */
    vec* commands;   /* vector of allowed commands */
};

typedef enum {
    CS_None,
    CS_Accepting,
    CS_Connected,
    CS_Closed,
    CS_Error,
} connection_state;

struct connection {
    int fd;
    connection_state state;
};

int configure_server(lexi_server* s, const char* input, size_t input_len,
                     vstr* error);

#endif /* __SERVER_H__ */
