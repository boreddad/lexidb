#include "cli-cmd.h"
#include "cmd.h"
#include "sock.h"
#include <stdio.h>
#include <string.h>

typedef enum { CCT_EOF, CCT_STRING, CCT_BULK, CCT_INT } CliTokenT;

typedef struct {
    char* input;
    size_t pos;
    size_t read_pos;
    size_t inp_len;
    char ch;
} CliLexer;

typedef struct {
    CliTokenT type;
    char* literal;
} CliToken;

typedef struct {
    CliTokenT exp;
    CliTokenT got;
} CliParserError;

typedef struct {
    size_t len;
    size_t cap;
    CliParserError* errs;
} CliParserErrors;

typedef struct {
    CliLexer l;
    CliToken cur_tok;
    CliToken peek_tok;
} CliParser;

void cli_lexer_read_char(CliLexer* l) {
    if (l->read_pos >= l->inp_len) {
        l->ch = 0;
    } else {
        l->ch = l->input[l->read_pos];
    }
    l->pos = l->read_pos;
    l->read_pos += 1;
}

void cli_lexer_read_until_space(CliLexer* l) {
    for (; ((l->ch != ' ') && (l->ch != '\0')); cli_lexer_read_char(l)) {
    }
}

void cli_lexer_read_bulk(CliLexer* l) {
    for (; ((l->ch != '"') && (l->ch != '\0')); cli_lexer_read_char(l)) {
    }
    cli_lexer_read_char(l);
}

CliToken cli_lexer_next_token(CliLexer* l) {
    CliToken tok = {0};

    switch (l->ch) {
    case ':':
        tok.type = CCT_INT;
        tok.literal = l->input + l->pos;
        cli_lexer_read_until_space(l);
        break;
    case '"':
        tok.type = CCT_BULK;
        tok.literal = l->input + l->pos;
        cli_lexer_read_char(l);
        cli_lexer_read_bulk(l);
        break;
    case '\0':
        tok.type = CCT_EOF;
        tok.literal = "\0";
        break;
    default:
        tok.type = CCT_STRING;
        tok.literal = l->input + l->pos;
        cli_lexer_read_until_space(l);
        break;
    }

    cli_lexer_read_char(l);

    return tok;
}

CliLexer cli_lexer_new(char* input, size_t input_len) {
    CliLexer l = {0};

    l.input = input;
    l.inp_len = input_len;

    cli_lexer_read_char(&l);

    return l;
}

void cli_parser_next_token(CliParser* p) {
    p->cur_tok = p->peek_tok;
    p->peek_tok = cli_lexer_next_token(&(p->l));
}

uint8_t cli_parser_peek_token_is(CliParser* p, CliTokenT type) {
    return p->peek_tok.type == type ? 1 : 0;
}

uint8_t cli_parser_expect_peek(CliParser* p, CliTokenT type) {
    if (cli_parser_peek_token_is(p, type)) {
        cli_parser_next_token(p);
        return 1;
    }
    return 0;
}

uint8_t cli_parser_cur_token_is(CliParser* p, CliTokenT type) {
    return p->cur_tok.type == type ? 1 : 0;
}

CliParser cli_parser_new(CliLexer* l) {
    CliParser p = {0};

    p.l = *l;

    cli_parser_next_token(&p);
    cli_parser_next_token(&p);

    return p;
}

CliCmdT cli_parser_parse_cmd_type(CliParser* p) {
    char* literal = p->cur_tok.literal;

    if (strncmp(literal, "set ", 4) == 0) {
        return CC_SET;
    }

    if (strncmp(literal, "get ", 4) == 0) {
        return CC_GET;
    }

    if (strncmp(literal, "del ", 4) == 0) {
        return CC_DEL;
    }

    if (strncmp(literal, "ping", 4) == 0) {
        return CC_PING;
    }

    if (strncmp(literal, "help", 4) == 0) {
        return CC_HELP;
    }

    return CC_INV;;
}

size_t cli_parser_get_string_len(CliToken* tok) {
    char* literal = tok->literal;
    size_t res = 0;
    while ((*literal != '\0') && (*literal != ' ')) {
        literal++;
        res++;
    }
    return res;
}

size_t cli_parser_get_bulk_string_len(CliToken* tok) {
    char* literal = tok->literal;
    size_t res = 0;

    literal++; // skip fist '"'

    while ((*literal != '\0') && (*literal != '"')) {
        literal++;
        res++;
    }

    return res;
}

int64_t cli_parser_parse_int(CliToken* tok) {
    int64_t res = 0;
    char* literal = tok->literal;
    literal++;
    while ((*literal != '\0') && (*literal != ' ')) {
        res = (res * 10) + (*literal - '0');
        literal++;
    }
    return res;
}

CliCmd cli_parser_parse_cmd(CliParser* p) {
    CliCmd cmd = {0};
    if (cli_parser_cur_token_is(p, CCT_STRING)) {
        cmd.type = cli_parser_parse_cmd_type(p);
        switch (cmd.type) {
        case CC_HELP:
            break;
        case CC_PING:
            break;
        case CC_SET:
            if (cli_parser_expect_peek(p, CCT_STRING)) {
                cmd.expr.set.key.len = cli_parser_get_string_len(&(p->cur_tok));;
                cmd.expr.set.key.value = ((uint8_t*)(p->cur_tok.literal));

                cli_parser_next_token(p);

                if (cli_parser_cur_token_is(p, CCT_BULK)) {
                    cmd.expr.set.value.type = VTSTRING;
                    cmd.expr.set.value.size = cli_parser_get_bulk_string_len(&(p->cur_tok));
                    cmd.expr.set.value.ptr = p->cur_tok.literal + 1; // +1 to skip first '"'
                } else if (cli_parser_cur_token_is(p, CCT_STRING)) {
                    cmd.expr.set.value.type = VTSTRING;
                    cmd.expr.set.value.size = cli_parser_get_string_len(&(p->cur_tok));
                    cmd.expr.set.value.ptr = p->cur_tok.literal;
                } else if (cli_parser_cur_token_is(p, CCT_INT)) {
                    cmd.expr.set.value.type = VTINT;
                    cmd.expr.set.value.size = sizeof(int64_t);
                    cmd.expr.set.value.ptr = ((void*)cli_parser_parse_int(&(p->cur_tok)));
                } else {
                    cmd.expr.set.value.type = VTNULL;
                    cmd.expr.set.value.size = 0;
                    cmd.expr.set.value.ptr = NULL;
                }
            } else {
                printf("invalid key\n");
                cmd.type = CC_INV;
            }
            break;
        case CC_GET:
            if (cli_parser_expect_peek(p, CCT_STRING)) {
                cmd.expr.get.key.len = cli_parser_get_string_len(&(p->cur_tok));;
                cmd.expr.get.key.value = ((uint8_t*)(p->cur_tok.literal));
            } else {
                printf("invalid key\n");
                cmd.type = CC_INV;
            }
            break;
        case CC_DEL:
            if (cli_parser_expect_peek(p, CCT_STRING)) {
                cmd.expr.del.key.len = cli_parser_get_string_len(&(p->cur_tok));;
                cmd.expr.del.key.value = ((uint8_t*)(p->cur_tok.literal));
            } else {
                printf("invalid key\n");
                cmd.type = CC_INV;
            }
            break;
        default:
            printf("invalid command\n");
            cmd.type = CC_INV;
            break;
        }
    } else {
        cmd.type = CC_INV;
        printf("invalid command\n");
    }

    return cmd;
}

void cli_print_cmd_type(CliCmdT type) {
    if (type == CC_SET) {
        printf("SET ");
        return;
    }
    if (type == CC_GET) {
        printf("GET ");
        return;
    }
    if (type == CC_DEL) {
        printf("DEL ");
        return;
    }
    if (type == CC_PING) {
        printf("PING");
        return;
    }
    if (type == CC_HELP) {
        printf("HELP");
        return;
    }
}

void cli_print_key(Key* key) {
    size_t i, len;
    len = key->len;

    for (i = 0; i < len; ++i) {
        printf("%c", key->value[i]);
    }
}

void cli_print_value(Value* val) {
    ValueT t = val->type;
    if (t == VTSTRING) {
        size_t i, len;
        char* s = ((char*)(val->ptr));
        len = val->size;
        for (i = 0; i < len; ++i) {
            printf("%c", s[i]);
        }
    } else if (t == VTINT) {
        int64_t v = ((int64_t)(val->ptr));
        printf("%ld", v);
    } else {
        printf("null");
    }
}

void cli_cmd_print(CliCmd* cmd) {
    cli_print_cmd_type(cmd->type);
    if (cmd->type == CC_SET) {
        cli_print_key(&(cmd->expr.set.key));
        printf(" ");
        cli_print_value(&(cmd->expr.set.value));
    }
    if (cmd->type == CC_GET) {
        cli_print_key(&(cmd->expr.get.key));
    }
    if (cmd->type == CC_DEL) {
        cli_print_key(&(cmd->expr.del.key));
    }
    printf("\n");
}

CliCmd cli_parse_cmd(char* input, size_t input_len) {
    CliLexer l = cli_lexer_new(input, input_len);
    CliParser p = cli_parser_new(&l);
    CliCmd cmd = cli_parser_parse_cmd(&p);
    return cmd;
}
