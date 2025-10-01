#include "corm.h"
#include "jacon.h"
#include <stdlib.h>
#include <stdio.h>

char* read_file(FILE *file)
{
    if (fseek(file, 0, SEEK_END) < 0) return NULL;
    long ft = ftell(file);
    if (ft < 0) return NULL;
    if (fseek(file, 0, SEEK_SET) < 0) return NULL;
    
    size_t file_size = ft + 1;
    char *file_content = calloc(file_size, 1);
    if(file_content == NULL) return NULL;

    if (fread(file_content, 1, ft, file) != (size_t) ft) return NULL;
    return file_content;
}

int corm_init_json_db(CormDatabase *db, char *file_path)
{
    if (!db || !file_path) return 1;
    
    db->type = JSON_DATABASE;
    db->json.file_path = file_path;

    FILE* f = fopen(file_path, "r");
    if (!f) return 1;
    char *raw_json = read_file(f);
    fclose(f);
    if (!raw_json) return 1;

    Jacon_init_content(&db->json.db);
    Jacon_deserialize(&db->json.db, raw_json);

    free(raw_json);
    return 0;
}

// TODO: Add support for use of multiple processes.
// Right now we erase the data someone else might have written.
int corm_commit_json_db(CormDatabase *db)
{
    FILE *f = fopen(db->json.file_path, "w");
    if (!f) return 1;
    char *ser_db = Jacon_serialize(db->json.db.root);

    fprintf(f, "%s", ser_db);

    free(ser_db);
    fclose(f);
    return 0;
}

int corm_commit(CormDatabase *db)
{
    switch (db->type)
    {
    case JSON_DATABASE:
        return corm_commit_json_db(db);
    }
    return 1;
}

void corm_free_json_database(CormDatabase *db)
{
    Jacon_free_content(&db->json.db);
}

void corm_free_database(CormDatabase *db)
{
    switch (db->type)
    {
    case JSON_DATABASE:
        corm_free_json_database(db);
        break;
    }
}