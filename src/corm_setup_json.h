#ifndef CORM_SETUP_JSON_H
#define CORM_SETUP_JSON_H

#include "corm_setup_types.h"
#include "corm_setup.h"
#include <stdio.h>

int setup_for_json_db(CormContext *context, ParsedStruct *s, FILE* f_header, FILE* f_source);

#endif // CORM_SETUP_JSON_H
