#ifndef CORM_SETUP_GLOBAL_H
#define CORM_SETUP_GLOBAL_H

#include "corm_setup_types.h"
#include <stdio.h>

int setup_free(ParsedStruct *s, FILE* f_header, FILE* f_source);
int setup_print(ParsedStruct *s, FILE* f_header, FILE* f_source);

#endif // CORM_SETUP_GLOBAL_H