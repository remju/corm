#include "../corm_setup.h"

// Setup file for your data types
// There are two ways of calling the setup function chose as you prefer

int main(void)
{
    CormContext context = {
        // Header file to be parsed
        .input_header = "example/user.h", 
        .output_path = "example/",  
        // CRUD functions to include (CREATE, READ, UPDATE, DELETE, CRUD for all)
        .crud = CRUD,                       
        // Include debug functions (print_*, ...)
        .options = DEBUG,                   
    };
    corm_setup_(context);    
    return 0;
}

int main2(void)
{
    corm_setup(.input_header = "example/user.h", .output_path = "example/", .crud = CRUD, .options = DEBUG);    
    return 0;
}