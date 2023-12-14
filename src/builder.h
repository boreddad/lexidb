#ifndef __BUILDER_H__

#define __BUILDER_H__

#include "vstr.h"

typedef vstr builder;

builder builder_new(void);
const uint8_t* builder_out(builder* b);
size_t builder_len(builder* b);
int builder_add_ok(builder* b);
int builder_add_pong(builder* b);
int builder_add_array(builder* b, size_t arr_len);
int builder_add_string(builder* b, const char* str, size_t str_len);
void builder_reset(builder* b);
void builder_free(builder* builder);

#endif /* __BUILDER_H__ */
