#include "config.h"

void encode_example(void)
{
    struct foo data = {.bar = 42, .baz = "hello"};
    char buffer[128];
    int ret;

    ret = json_obj_encode_buf(foo_descr, ARRAY_SIZE(foo_descr),
                              &data,
                              buffer, sizeof(buffer));
    if (ret < 0)
    {
        /* handle error */
    }
}

void decode_example(void)
{
    struct foo data;
    const char *json_text = "{\"bar\": 42, \"baz\": \"hello\"}";
    int ret;

    ret = json_obj_parse(json_text, strlen(json_text),
                         foo_descr, ARRAY_SIZE(foo_descr),
                         &data);
    if (ret < 0)
    {
        /* handle error */
    }
}