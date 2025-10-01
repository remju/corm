#ifndef CORM_SETUP_UTILS_H
#define CORM_SETUP_UTILS_H

#include <stdarg.h>

#ifndef TMP_STR_SIZE
#define TMP_STR_SIZE    1024
#endif // TMP_STR_SIZE

#define LOG(lvl, fmt, ...) (printf("[%s] " fmt "\n", #lvl, ##__VA_ARGS__))

const char* tmp_str(const char *fmt, ...);

#endif // CORM_SETUP_UTILS_H