#ifndef CORM_SETUP_H
#define CORM_SETUP_H

#include "corm.h"

#define DEBUG           0x01

#define CREATE          0x01
#define READ            0x02
#define UPDATE          0x04
#define DELETE          0x08
#define CRUD            (CREATE|READ|UPDATE|DELETE)

typedef struct {
    char *input_header;
    char *output_path;
    int options;
    int crud;
    int db_type;
    
    // Internal, do not touch
    char *input_file_content;
    size_t input_file_size;
} CormContext;

#define corm_setup_(...) (corm_setup((CormContext) { __VA_ARGS__ }))
int corm_setup(CormContext context);

#endif // CORM_SETUP_H