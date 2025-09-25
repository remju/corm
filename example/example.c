#include "corm_generated.h"
#include "../Jacon/jacon.h"
#include <stdio.h>
#include <stdlib.h>

#define str_bool(bool) (bool ? "true" : "false")
#define DB_PATH "example/users.json"

char* read_file(FILE *file, size_t *file_size);
int save_db(const char *db_path, Jacon_content *json_db);

int main(void)
{
    // Setup the database
    size_t file_size = 0;
    FILE* f = fopen(DB_PATH, "rb");
    char *raw_json = read_file(f, &file_size);
    Jacon_content json_db = {0};
    Jacon_init_content(&json_db);
    Jacon_deserialize(&json_db, raw_json);
    free(raw_json);
    fclose(f);

    // Read one user
    {
        User user = {0};
        char *user_id = "1";
        read_User(&json_db, &user, user_id);
        printf("user: %s\n\tlogin: %s\n\tpassword: %s\n\tage: %d\n\theight: %f\n\tmarried: %s\n",
            user_id, user.login, user.password, user.age, user.height, str_bool(user.married));
        free_User(&user);
    }

    // Create a new user
    {
        User new_user = {
            .login = "new",
            .password = "password",
            .age = 21,
            .height = 86.4,
            .married = false
        };
        create_User(&json_db, &new_user, "3");
    }

    // Update a user
    {
        User updated_user = {0};
        read_User(&json_db, &updated_user, "1");
        free(updated_user.password);
        updated_user.password = "updatedpassword"; // Memory leak
        update_User(&json_db, &updated_user, "1");
        free(updated_user.login);

        User user = {0};
        char *user_id = "1";
        read_User(&json_db, &user, user_id);
        printf("user: %s\n\tlogin: %s\n\tpassword: %s\n\tage: %d\n\theight: %f\n\tmarried: %s\n",
            user_id, user.login, user.password, user.age, user.height, str_bool(user.married));
        free_User(&user);
    }

    // Delete a user
    delete_User(&json_db, "1");

    // Save the db to json file to keep modifications
    // save_db(DB_PATH, &json_db);

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
    
    (*file_size)++; // Extra null byte
    char *file_content = calloc(*file_size, 1);
    if(file_content == NULL) return NULL;

    fread(file_content, *file_size, 1, file);
    return file_content;
}

int save_db(const char *db_path, Jacon_content *json_db)
{
    FILE *f = fopen(DB_PATH, "wb");
    char *ser_db = Jacon_serialize(json_db->root);
    fprintf(f, "%s", ser_db);
    free(ser_db);
    fclose(f);
    return 0;
}