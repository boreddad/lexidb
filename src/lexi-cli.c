#define _XOPEN_SOURCE 600
#include "cmd.h"
#include "hilexi.h"
#include "line_parser.h"
#include "log.h"
#include "object.h"
#include "read_line.h"
#include "vstr.h"
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>

#define PROMPT "> "

int main(void) {
    result(hilexi) rl = hilexi_new("127.0.0.1", 6969);
    hilexi l;
    int connect_res;
    if (rl.type == Err) {
        error("%s", vstr_data(&(rl.data.err)));
        return 1;
    }
    l = rl.data.ok;
    connect_res = hilexi_connect(&l);
    if (connect_res == -1) {
        error("failed to connect (errno: %d) %s\n", errno, strerror(errno));
        hilexi_close(&l);
        return 1;
    }
    for (;;) {
        vstr line;
        const char* line_data;
        size_t line_len;
        cmd cmd;
        result(object) cmd_res;
        object obj;

        printf(PROMPT);
        fflush(stdout);

        line = read_line();
        line_data = vstr_data(&line);
        line_len = vstr_len(&line);

        if (line_len == 4 && memcmp(line_data, "exit", 4) == 0) {
            vstr_free(&line);
            break;
        }

        cmd = parse_line(line_data, line_len);
        switch (cmd.type) {
        case Ping:
            cmd_res = hilexi_ping(&l);
            break;
        case Set: {
            set_cmd set = cmd.data.set;
            object key = set.key;
            object value = set.value;
            cmd_res = hilexi_set(&l, &key, &value);
        } break;
        case Get: {
            get_cmd get = cmd.data.get;
            object key = get.key;
            cmd_res = hilexi_get(&l, &key);
        } break;
        case Del: {
            del_cmd del = cmd.data.del;
            object key = del.key;
            cmd_res = hilexi_del(&l, &key);
        } break;
        default:
            cmd_res.type = Err;
            cmd_res.data.err = vstr_from("invalid command");
            break;
        }
        vstr_free(&line);
        if (cmd_res.type == Err) {
            error("%s\n", vstr_data(&(cmd_res.data.err)));
            vstr_free(&(cmd_res.data.err));
            continue;
        }

        obj = cmd_res.data.ok;
        object_show(&obj);
        object_free(&obj);
    }
    hilexi_close(&l);
    return 0;
}
