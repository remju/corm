#include "corm_generated.h"
#include "../Jacon/jacon.h"
#include <stdio.h>
#include <stdlib.h>

#define str_bool(bool) (bool ? "true" : "false")

char* read_file(FILE *file, size_t *file_size);

int main(void)
{
    // Setup the database
    size_t file_size = 0;
    FILE* f = fopen("example/users.json", "rb");
    char *raw_json = read_file(f, &file_size);
    Jacon_content json_db = {0};
    Jacon_init_content(&json_db);
    Jacon_deserialize(&json_db, raw_json);

    // Retrieve 1 user
    User user = {0};
    char *user_id = "1";
    read_User(&json_db, &user, user_id);
    printf("user: %s\n\tlogin: %s\n\tpassword: %s\n\tage: %d\n\theight: %f\n\tmarried: %s\n",
        user_id, user.login, user.password, user.age, user.height, str_bool(user.married));

    free_User(&user);

    user_id = "2";
    read_User(&json_db, &user, user_id);
    printf("user: %s\n\tlogin: %s\n\tpassword: %s\n\tage: %d\n\theight: %f\n\tmarried: %s\n",
        user_id, user.login, user.password, user.age, user.height, str_bool(user.married));

    free_User(&user);

    // Free database ressources
    Jacon_free_content(&json_db);

    return 0;
}

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