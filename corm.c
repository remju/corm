#include "corm.h"
#include "Jacon/jacon.h"
#include <stdlib.h>
#include <stdio.h>

char* read_file(FILE *file)
{
    if (fseek(file, 0, SEEK_END) < 0) return NULL;
    size_t file_size = ftell(file);
    if (file_size < 0) return NULL;
    if (fseek(file, 0, SEEK_SET) < 0) return NULL;
    
    file_size++; // Extra null byte
    char *file_content = calloc(file_size, 1);
    if(file_content == NULL) return NULL;

    fread(file_content, file_size, 1, file);
    return file_content;
}

// int save_db(const char *db_path, Jacon_content *json_db)
// {
//     FILE *f = fopen(db_path, "wb");
//     char *ser_db = Jacon_serialize(json_db->root);
//     fprintf(f, "%s", ser_db);
//     free(ser_db);
//     fclose(f);
//     return 0;
// }

int corm_init_json_db(CormDatabase *db, char *file_path)
{
    if (!db || !file_path) return 1;
    
    db->type = JSON_DATABASE;
    db->json.file_path = file_path;

    FILE* f = fopen(file_path, "rb");
    char *raw_json = read_file(f);

    Jacon_init_content(&db->json.db);
    Jacon_deserialize(&db->json.db, raw_json);

    free(raw_json);
    fclose(f);
    return 0;
}

// TODO: Add support for use of multiple processes.
// Right now we erase the data someone else might have written.
int corm_commit_json_db(CormDatabase *db)
{
    FILE *f = fopen(db->json.file_path, "wb");
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