#ifndef CORM_SETUP_H
#define CORM_SETUP_H

#define DEBUG           0x01

#define CREATE          0x01
#define READ            0x02
#define UPDATE          0x04
#define DELETE          0x08
#define CRUD            (CREATE|READ|UPDATE|DELETE)

#define TMP_STR_SIZE    1024

typedef struct {
    char *input_header;
    char *output_path;
    int options;
    int crud;
} CormContext;

#define corm_setup(...) (corm_setup_((CormContext) { __VA_ARGS__ }))
int corm_setup_(CormContext context);

#endif // CORM_SETUP_H