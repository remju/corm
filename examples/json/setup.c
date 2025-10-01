#include "../../src/corm_setup.h"

// Setup file for your data types
// There are two ways of calling the setup function chose as you prefer

int main(void)
{
    CormContext context = {
        // Header file to be parsed
        .input_header = "examples/json/user.h",
        // Output path or generated files
        .output_path = "build/",  
        // CRUD functions to include (CREATE, READ, UPDATE, DELETE, CRUD for all)
        .crud = CRUD,                       
        // Include debug functions (print_*, ...)
        .options = DEBUG,
        // Type of database you plan to use
        .db_type = JSON_DATABASE,                   
    };
    if (corm_setup(context)) return 1;    
    return 0;
}

int main2(void)
{
    if (corm_setup_(
        .input_header = "example/json/user.h", 
        .output_path = "build/", 
        .crud = CRUD, 
        .options = DEBUG,
        .db_type = JSON_DATABASE
    )) return 1;    
    return 0;
}