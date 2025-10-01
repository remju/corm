#ifndef CORM_SETUP_TYPES_H
#define CORM_SETUP_TYPES_H

#include <stddef.h>

#define TYPE_CONST      0x01
#define TYPE_UNSIGNED   0x02
#define TYPE_CHAR       0x04
#define TYPE_INT        0x08
#define TYPE_FLOAT      0x10
#define TYPE_DOUBLE     0x20
#define TYPE_BOOL       0x40
#define TYPE_POINTER    0x80

typedef struct {
    int type;
    char *name;
} Field;

typedef struct {
    char *name;             // Struct name with 1st char upper
    size_t name_len;        // Struct name with all chars lower, used for variable names
    char *lower_name;       // Name length
    Field **fields;
    size_t field_count;
    size_t field_capacity;
} ParsedStruct;

#endif // CORM_SETUP_TYPES_H