#include "corm.h"
#include "corm_user.h"
#include <stdlib.h>

int main(void)
{
    // Setup the database
    CormDatabase db = {0};
    if (corm_init_json_db(&db, "examples/json/users.json")) return 1;

    // Read one user
    {
        User first_user = {0};
        char *user_id = "1";
        read_user(&db, &first_user, user_id);
        print_user(first_user);
        free_user(&first_user);
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
        create_user(&db, &new_user, "3");
    }

    // Update a user
    {
        User updated_user = {0};
        char *user_id = "2";
        read_user(&db, &updated_user, user_id);
        free(updated_user.password);
        updated_user.password = "updatedpassword"; // Memory leak if no free before replacing
        update_user(&db, &updated_user, user_id);
        free(updated_user.login);

        User second_user = {0};
        read_user(&db, &second_user, user_id);
        print_user(second_user);
        free_user(&second_user);
    }
    
    // Delete a user
    delete_user(&db, "1");
  
    // Commit changes, changes are sent to real db
    // corm_commit(&db);

    // Free database ressources
    corm_free_database(&db);
    return 0;
}