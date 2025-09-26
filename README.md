# CORM - C Object-Relational Mapping

A lightweight ORM (Object-Relational Mapping) library for C that provides automated CRUD (Create, Read, Update, Delete) operations for C structures. CORM currently supports JSON file databases and automatically generates type-safe database operations from your C struct definitions.

## Features

- **Code Generation**: Automatically generates CRUD functions from C struct definitions
- **JSON Database Support**: Built-in support for JSON file databases via the [Jacon](https://github.com/remju/Jacon) JSON library
- **Debug Support**: Optional debug functions (print functions) for development
- **Simple API**: Clean, easy-to-use interface

## Quick Start

### 1. Define Your Data Structure

Create a header file with your data structures:

```c
// user.h
#include <stdbool.h>

typedef struct {
    char *login;
    char *password;
    unsigned int age;
    double height;
    bool married;
} User;
```

### 2. Generate CRUD Functions

Create a setup program to generate the ORM functions:

```c
// setup.c
#include "corm_setup.h"

int main(void)
{
    CormContext context = {
        // Header file to be parsed
        .input_header = "example/user.h",
        // Output path or generated files
        .output_path = "example/",  
        // CRUD functions to include (CREATE, READ, UPDATE, DELETE, CRUD for all)
        .crud = CRUD,                       
        // Include debug functions (print_*, ...)
        .options = DEBUG,
        // Type of database you plan to use
        .db_type = JSON_DATABASE,                   
    };
    corm_setup(context);    
    return 0;
}
```

Compile and run the setup:
```bash
gcc -o setup setup.c corm_setup.c
./setup
```

### 3. Use Generated Functions

```c
// main.c
#include "corm.h"
#include "corm_user.h"  // Generated file

int main(void) {
    // Initialize database
    CormDatabase db = {0};
    corm_init_json_db(&db, "users.json");

    // Create a new user
    User new_user = {
        .login = "alice",
        .password = "secret123",
        .age = 25,
        .height = 165.5,
        .married = false
    };
    create_user(&db, &new_user, "1");

    // Read a user
    User user = {0};
    read_user(&db, &user, "1");
    print_user(user);  // Debug function
    
    // Update the user
    user.age = 26;
    update_user(&db, &user, "1");

    // Delete a user
    delete_user(&db, "1");

    // Commit changes to file
    corm_commit(&db);

    // Cleanup
    free_user(&user);
    corm_free_database(&db);
    return 0;
}
```

## Building

### Prerequisites

- GCC compiler
- Make

### Build Example

```bash
make example
```

This will:
1. Build and run the setup program to generate CRUD functions
2. Compile the example program with the generated code

## Generated Functions

For each struct (e.g., `User`), CORM generates the following functions according to the given CRUD options:

### CRUD Operations
- `create_<type>(CormDatabase *db, <Type> *obj, char *id)` - Create a new record
- `read_<type>(CormDatabase *db, <Type> *obj, char *id)` - Read a record by ID
- `update_<type>(CormDatabase *db, <Type> *obj, char *id)` - Update an existing record
- `delete_<type>(CormDatabase *db, char *id)` - Delete a record by ID

### Memory Management
- `free_<type>(<Type> *obj)` - Free memory allocated for struct fields

### Debug Functions (when DEBUG option enabled)
- `print_<type>(<Type> obj)` - Print struct contents

## Database Types

### JSON Database
CORM currently supports JSON file databases. Data is stored in JSON format:

```json
{
  "1": {
    "login": "alice",
    "password": "secret123",
    "age": 25,
    "height": 165.5,
    "married": false
  },
  "2": {
    "login": "bob",
    "password": "password456",
    "age": 30,
    "height": 180.0,
    "married": true
  }
}
```

## Setup Options

### CRUD Options
- `CREATE` - Generate create functions
- `READ` - Generate read functions  
- `UPDATE` - Generate update functions
- `DELETE` - Generate delete functions
- `CRUD` - Generate all CRUD functions

### Other Options
- `DEBUG` - Generate debug/print functions

### Database Types
- `JSON_DATABASE` - JSON file database support

## Project Structure

```
corm/
├── corm.h              # Main CORM header
├── corm.c              # Core CORM implementation
├── corm_setup.h        # Setup/code generation header
├── corm_setup.c        # Setup/code generation implementation
├── stb_c_lexer.h       # C lexer for parsing headers
├── Jacon/              # JSON parsing library
├── example/            # Example usage
│   ├── user.h          # Example struct definition
│   ├── setup.c         # Setup program
│   ├── example.c       # Example usage
│   └── users.json      # Example JSON database
└── Makefile
```

## Dependencies

- [Jacon](https://github.com/remju/Jacon): JSON parsing library (included as subproject)
- [stb_c_lexer](https://github.com/nothings/stb/blob/master/stb_c_lexer.h): C lexer for parsing header files (included)

## License

See [LICENSE](LICENSE) file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if you feel the need (no tests currently available)
5. Submit a pull request

## Roadmap

- [ ] Support for additional database types (SQLite, etc.)
- [ ] Query builder functionality
- [ ] Relationship mapping between structs
- [ ] Transaction support
- [ ] Multi-process safety improvements
- [ ] Automatic memory management for generated functions


## Notes

- Currently designed for single-process usage
- JSON database commits overwrite the entire file
- Memory management is important - always call `free_<type>()` functions
- Generated files are prefixed with `corm_` (e.g., `corm_user.h`, `corm_user.c`)