#include <zephyr/data/json.h>

struct foo
{
    int bar;
    const char *baz;
};

static const struct json_obj_descr foo_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct foo, bar, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct foo, baz, JSON_TOK_STRING),
};