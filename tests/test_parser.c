#include "../src/cmd.h"
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/token.h"
#include "../src/builder.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_it_works() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    ArrayStatement astmt;

    uint8_t* input =
        ((uint8_t*)"*2\r\n*3\r\n$3\r\nSET\r\n$5\r\nvince\r\n$7\r\nis "
                   "cool\r\n*2\r\n$3\r\nGET\r\n$5\r\nvince\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    assert(cmd_ir.stmt.type == SARR);

    astmt = cmd_ir.stmt.statement.arr;
    assert(astmt.len == 2);

    Statement stmt0 = astmt.statements[0];
    Cmd cmd0 = cmd_from_statement(&stmt0);
    assert(cmd0.type == SET);
    assert(cmd0.expression.set.key.len == 5);
    assert(cmd0.expression.set.value.size == 7);

    Statement stmt1 = astmt.statements[1];
    Cmd cmd1 = cmd_from_statement(&stmt1);
    assert(cmd1.type == GET);
    assert(cmd0.expression.get.key.len == 5);

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);
    printf("parser test 1 passed (it works)\n");
}

void test_simple_string() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    size_t e_len;

    parser_toggle_debug(0);

    uint8_t* input =
        ((uint8_t*)"*2\n*3\r\n$3\r\nSET\r\n$5\r\nvince\r\n$7\r\nis "
                   "cool\r\n*2\r\n$3\r\nGET\r\n$5\r\nvince\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    e_len = parser_errors_len(&p);

    assert(e_len == 1);

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);
    printf("parser test 2 passed (arr missing retcar after len)\n");
}

void test_integers() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    size_t e_len;

    parser_toggle_debug(0);

    uint8_t* input =
        ((uint8_t*)"*2\r*3\r\n$3\r\nSET\r\n$5\r\nvince\r\n$7\r\nis "
                   "cool\r\n*2\r\n$3\r\nGET\r\n$5\r\nvince\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    e_len = parser_errors_len(&p);

    assert(e_len == 1);

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);
    printf("parser test 3 passed (arr missing newl after len)\n");
}

void t4() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    size_t e_len;

    parser_toggle_debug(0);

    uint8_t* input =
        ((uint8_t*)"*\r\n*3\r\n$3\r\nSET\r\n$5\r\nvince\r\n$7\r\nis "
                   "cool\r\n*2\r\n$3\r\nGET\r\n$5\r\nvince\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    e_len = parser_errors_len(&p);

    assert(e_len == 1);

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);
    printf("parser test 4 passed (arr missing len)\n");
}

void t5() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    size_t e_len;

    parser_toggle_debug(0);

    uint8_t* input =
        ((uint8_t*)"3\r\n*3\r\n$3\r\nSET\r\n$5\r\nvince\r\n$7\r\nis "
                   "cool\r\n*2\r\n$3\r\nGET\r\n$5\r\nvince\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    e_len = parser_errors_len(&p);

    assert(e_len == 1);

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);
    printf("parser test 5 passed (missing arr type)\n");
}

void t6() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    Cmd cmd;

    uint8_t* input = ((uint8_t*)"+PING\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    assert(cmd_ir.stmt.type == SPING);
    assert(cmd_ir.stmt.statement.sst == SST_PING);

    cmd = cmd_from_statement(&(cmd_ir.stmt));
    assert(cmd.type == CPING);

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);

    printf("parser test 6 passed (simple ping)\n");
}

void t7() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    size_t e_len;

    parser_toggle_debug(0);

    uint8_t* input =
        ((uint8_t*)"*3\r\n3\r\nSET\r\n$5\r\nvince\r\n$7\r\nis cool\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    e_len = parser_errors_len(&p);

    assert(e_len > 0); // e_len should equal 3 for some reason

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);
    printf("parser test 7 passed (missing str type)\n");
}

void t8() {
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    size_t e_len;

    parser_toggle_debug(0);

    uint8_t* input =
        ((uint8_t*)"*3\r\n$\r\nSET\r\n$5\r\nvince\r\n$7\r\nis cool\r\n");
    size_t inp_len = strlen(((char*)input));

    l = lexer_new(input, inp_len);
    p = parser_new(&l);

    cmd_ir = parse_cmd(&p);

    e_len = parser_errors_len(&p);

    assert(e_len > 0); // e_len = 3

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);
    printf("parser test 8 passed (missing str len)\n");
}

void t9() {
    Builder b = builder_create(32);
    size_t buf_len;
    uint8_t* buf;
    Lexer l;
    Parser p;
    CmdIR cmd_ir;
    Cmd cmd;
    int64_t v;
    builder_add_arr(&b, 3);
    builder_add_string(&b, "SET", 3);
    builder_add_string(&b, "vince", 5);
    builder_add_int(&b, 42069);

    buf_len = b.ins;
    buf = builder_out(&b);

    l = lexer_new(buf, buf_len);
    p = parser_new(&l);
    cmd_ir = parse_cmd(&p);

    assert(cmd_ir.stmt.type == SARR);

    cmd = cmd_from_statement(&(cmd_ir.stmt));
    assert(cmd.expression.set.value.type == VTINT);
    v = ((int64_t)(cmd.expression.set.value.ptr));
    assert(v == 42069);

    cmdir_free(&cmd_ir);
    parser_free_errors(&p);

    printf("parser test 9 passsed (integers)\n");
}


int main(void) {
    test_it_works();
    test_simple_string();
    test_integers();
    t4();
    t5();
    t6();
    t7();
    t8();
    t9();
    printf("all parser tests passed\n");
    return 0;
}
