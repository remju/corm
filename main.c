// #include "corm.h"
#include "./Jacon/jacon.h"
#include <stdio.h>
#include <stdlib.h>

char* read_file(FILE *file, size_t *file_size)
{
    if (fseek(file, 0, SEEK_END) < 0) return NULL;
    *file_size = ftell(file);
    if (*file_size < 0) return NULL;
    if (fseek(file, 0, SEEK_SET) < 0) return NULL;
    
    char *file_content = calloc(*file_size, 1);
    if(file_content == NULL) return NULL;

    fread(file_content, *file_size, 1, file);
    return file_content;
}

int main(void)
{
    size_t file_size = 0;
    FILE* f = fopen("examples/users.json", "rb");
    char *raw_json = read_file(f, &file_size);

    Jacon_content json = {0};
    Jacon_init_content(&json);
    Jacon_deserialize(&json, raw_json);

    free(raw_json);

    char* login;
    Jacon_get_string_by_name(&json, "1.login", &login);
    printf("login: %s\n", login);
    free(login);
    
    Jacon_get_string_by_name(&json, "2.login", &login);
    printf("login: %s\n", login);
    free(login);

    Jacon_free_content(&json);
    return 0;
}