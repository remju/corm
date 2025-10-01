#ifndef CORM_H
#define CORM_H

#include "jacon.h"

enum DatabaseTypes
{
    JSON_DATABASE = 1,
    SQLITE_DATABASE,
    // More to come
};

typedef struct {
    int type;
    union
    {
        struct { char *file_path; Jacon_content db; } json;
    };
} CormDatabase;

int corm_init_json_db(CormDatabase *db, char *file_path);

int corm_commit(CormDatabase *db);

void corm_free_database(CormDatabase *db);

#endif //CORM_H