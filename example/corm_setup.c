#include "../corm.h"

int main(void)
{
    static const char *file_path = "example/user.h";
    corm_setup(file_path);    
    return 0;
}