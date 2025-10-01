#include "corm_setup_utils.h"
#include <stdio.h>

const char* tmp_str(const char *fmt, ...)
{
    static char tmp_str_buf[TMP_STR_SIZE];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp_str_buf, fmt, args);
    va_end(args);
    return tmp_str_buf;
}