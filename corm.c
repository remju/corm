#include "corm.h"
#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(lvl, fmt, ...) (printf("[%s] " fmt "\n", #lvl, ##__VA_ARGS__))

typedef struct {
    int type;
    char *name;
} Field;

typedef struct {
    char *name;
    Field **fields;
    size_t field_count;
    size_t field_capacity;
} ParsedStruct;

#define TYPE_CONST      0x01
#define TYPE_UNSIGNED   0x02
#define TYPE_CHAR       0x04
#define TYPE_INT        0x08
#define TYPE_FLOAT      0x10
#define TYPE_DOUBLE     0x20
#define TYPE_BOOL       0x40
#define TYPE_POINTER    0x80

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

int parse_type(const char *type)
{
    if (strcmp(type, "const") == 0) return TYPE_CONST;
    if (strcmp(type, "unsigned") == 0) return TYPE_UNSIGNED;
    if (strcmp(type, "char") == 0) return TYPE_CHAR;
    if (strcmp(type, "int") == 0) return TYPE_INT;
    if (strcmp(type, "float") == 0) return TYPE_FLOAT;
    if (strcmp(type, "double") == 0) return TYPE_DOUBLE;
    if (strcmp(type, "pointer") == 0) return TYPE_POINTER;
    if (strcmp(type, "bool") == 0) return TYPE_BOOL;
    return 0;
}

int parse_field(Field **field, stb_lexer *lexer)
{
    int parsed_type;
    char *last_id;
    stb_c_lexer_get_token(lexer);
    if (lexer->token != CLEX_id) return 0;
    *field = calloc(1, sizeof(Field));
    while (lexer->token != 59) // 59: ';'
    {
        if (lexer->token == 42) // 42: '*'
        {
            (*field)->type += TYPE_POINTER;
        }
        else
        {
            parsed_type = parse_type(lexer->string);
            (*field)->type += parsed_type;
            last_id = lexer->string;
        }
        stb_c_lexer_get_token(lexer);
    }
    (*field)->name = strdup(lexer->string);
    return 0;
}

int store_field(ParsedStruct *s, Field *f)
{
    size_t new_count = s->field_count + 1;
    if (new_count > s->field_capacity)
    {
        size_t new_capacity = s->field_capacity == 0 ? 2 : s->field_capacity * 2;
        s->fields = realloc(s->fields, new_capacity * sizeof(Field*));
        if (!s->fields) return 1;
        s->field_capacity = new_capacity;
    }
    s->fields[s->field_count++] = f;
    return 0;
}

int parse_struct(ParsedStruct *s, const char *file_content, size_t file_size)
{
    stb_lexer lexer = {0};
    static char string_store[1024];
    stb_c_lexer_init(&lexer, file_content, file_content + file_size, string_store, sizeof(string_store));

    if (!stb_c_lexer_get_token(&lexer)) return 1;
    if (lexer.token == CLEX_id && strcmp(lexer.string, "typedef") == 0)
    {
        if (!stb_c_lexer_get_token(&lexer)) return 1;
        if (lexer.token == CLEX_id && strcmp(lexer.string, "struct") == 0)
        {
            stb_c_lexer_get_token(&lexer);
            while(1)
            {
                Field *field = NULL;
                parse_field(&field, &lexer);
                if (!field) break;
                store_field(s, field);
            }
            stb_c_lexer_get_token(&lexer);
            s->name = strdup(lexer.string);
        }
    }

    return 0;
}

int setup_read(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating read function for struct: %s", s->name);

    char var_name[64];
    sprintf(var_name, "var_%s", s->name);

    fprintf(f_header, "void read_%s(Jacon_content *db, %s *%s, const char *%s_key);\n\n", s->name, s->name, var_name, s->name);
    fprintf(f_source, "void read_%s(Jacon_content *db, %s *%s, const char *%s_key)\n{\n", s->name, s->name, var_name, s->name);
    fprintf(f_source, "    char key[128];\n");

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;       
        // TODO: Introduce a tmp string making method to avoid using the next line for every fields
        fprintf(f_source, "    sprintf(key, \"%%s.%s\", User_key);\n", field_name);
        if (field_type & TYPE_CHAR && field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    Jacon_get_string_by_name(db, key, &%s->%s);\n", var_name, field_name);
        }
        else if (field_type & TYPE_INT)
        {
            fprintf(f_source, "    Jacon_get_int_by_name(db, key, &%s->%s);\n", var_name, field_name);
        }
        else if (field_type & TYPE_FLOAT)
        {
            fprintf(f_source, "    Jacon_get_float_by_name(db, key, &%s->%s);\n", var_name, field_name);
        }
        else if (field_type & TYPE_DOUBLE)
        {
            fprintf(f_source, "    Jacon_get_double_by_name(db, key, &%s->%s);\n", var_name, field_name);
        }
        else if (field_type & TYPE_BOOL)
        {
            fprintf(f_source, "    Jacon_get_bool_by_name(db, key, &%s->%s);\n", var_name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_create(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating create function for struct: %s", s->name);

    char var_name[64];
    sprintf(var_name, "var_%s", s->name);

    fprintf(f_header, "void create_%s(Jacon_content *db, %s *%s, const char *%s_key);\n\n", s->name, s->name, var_name, s->name);
    fprintf(f_source, "void create_%s(Jacon_content *db, %s *%s, const char *%s_key)\n{\n", s->name, s->name, var_name, s->name);

    fprintf(f_source, "    Jacon_Node *node = calloc(1, sizeof(Jacon_Node));\n");
    fprintf(f_source, "    node->name = strdup(%s_key);\n", s->name);
    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;
        fprintf(f_source, "    Jacon_Node *%s = calloc(1, sizeof(Jacon_Node));\n", field_name);
        fprintf(f_source, "    %s->name = strdup(\"%s\");\n", field_name, field_name);
        if (field_type & TYPE_CHAR && field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_STRING;\n", field_name);
            fprintf(f_source, "    %s->value.string_val = strdup(%s->%s);\n", field_name, var_name, field_name);
        }
        else if (field_type & TYPE_INT)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_INT;\n", field_name);
            fprintf(f_source, "    %s->value.int_val = %s->%s;\n", field_name, var_name, field_name);
        }
        else if (field_type & TYPE_FLOAT)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_FLOAT;\n", field_name);
            fprintf(f_source, "    %s->value.float_val = %s->%s;\n", field_name, var_name, field_name);
        }
        else if (field_type & TYPE_DOUBLE)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_DOUBLE;\n", field_name);
            fprintf(f_source, "    %s->value.double_val = %s->%s;\n", field_name, var_name, field_name);
        }
        else if (field_type & TYPE_BOOL)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_BOOLEAN;\n", field_name);
            fprintf(f_source, "    %s->value.bool_val = %s->%s;\n", field_name, var_name, field_name);
        }
        fprintf(f_source, "    Jacon_append_child(node, %s);\n", field_name);
    }

    fprintf(f_source, "    Jacon_append_child(db->root, node);\n");
    fprintf(f_source, "    Jacon_add_node_to_map(&db->entries, node, NULL);\n}\n\n");
    return 0;
}

int setup_update(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating update function for struct: %s", s->name);

    char var_name[64];
    sprintf(var_name, "var_%s", s->name);

    fprintf(f_header, "void update_%s(Jacon_content *db, %s *%s, const char *%s_key);\n\n", s->name, s->name, var_name, s->name);
    fprintf(f_source, "void update_%s(Jacon_content *db, %s *%s, const char *%s_key)\n{\n", s->name, s->name, var_name, s->name);

    // Working with json, we do not need to care about SQL keys relations (PK,FK).
    // With SQL it would be needed to overwrite existing data, here delete and create is fine.
    // This introduces the following thing to work on.
    // TODO: Abstract the underlying storage system from CRUD functions
    //       This function should be the same for SQL, json, ...
    //       Maybe it is the time to add a context to our lib.
    //       Either have the storage defined at compile time or leave the
    //       choice to the user to define and use multiple ones at runtime.

    // TODO:
    // We do not need to delete the keys from the map as they will be overwritten
    // Introduce another method to only remove node from root's childs and put another at its location
    // This skips useless steps of removing from map - better perfs
    fprintf(f_source, "    delete_User(db, %s_key);\n", s->name);
    fprintf(f_source, "    create_User(db, %s, %s_key);\n", var_name, s->name);
    
    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_delete(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating delete function for struct: %s", s->name);

    fprintf(f_header, "void delete_%s(Jacon_content *db, const char *%s_key);\n\n", s->name, s->name);
    fprintf(f_source, "void delete_%s(Jacon_content *db, const char *%s_key)\n{\n", s->name, s->name);

    fprintf(f_source, "    char key[128];\n");
    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        // TODO: Introduce a tmp string making method to avoid using the next line for every fields
        fprintf(f_source, "    sprintf(key, \"%%s.%s\", %s_key);\n", field_name, s->name);
        fprintf(f_source, "    Jacon_hm_remove(&db->entries, key);\n");
    }

    // TODO: Remove node from root's childs

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_free(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating free function for struct: %s", s->name);

    char var_name[64];
    sprintf(var_name, "var_%s", s->name);

    fprintf(f_header, "void free_%s(%s *%s);\n\n", s->name, s->name, var_name);
    fprintf(f_source, "void free_%s(%s *%s)\n{\n", s->name, s->name, var_name);

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;

        if (field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    free(%s->%s);\n", var_name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");
    return 0;
}

int corm_setup(const char *input_path)
{
    LOG(INFO, "Starting corm setup");
    FILE *f = fopen(input_path, "rb");
    if (!f) return 1;

    size_t file_size = 0;
    char *file_content = read_file(f, &file_size);
    fclose(f);
    if (!file_content)
    {
        return 1;
    }

    ParsedStruct s = {0};
    parse_struct(&s, file_content, file_size);

    // TODO: Rename those files with the struct's name to avoid overwriting if we have multiple structs.
    FILE* f_header = fopen("example/corm_generated.h", "wb");
    if (!f_header) return 1;
    FILE* f_source = fopen("example/corm_generated.c", "wb");
    if (!f_source) return 1;

    fprintf(f_header, "#ifndef CORM_GENERATED_H\n#define CORM_GENERATED_H\n\n");
    // TODO: Detect custom types to include correct files.
    // Lazy workaround, leave the responsability to give the header files to the user
    // by letting him include them in its .h file so we can get them.
    fprintf(f_header, "#include <stddef.h>\n#include <stdbool.h>\n#include \"../Jacon/jacon.h\"\n\n");
    fprintf(f_header, "%s\n\n", file_content);
    fprintf(f_source, "#include \"corm_generated.h\"\n#include <stdio.h>\n#include <stdlib.h>\n\n");

    // TODO: Leave the choice to setup these to the user ?
    setup_read(&s, f_header, f_source);
    setup_create(&s, f_header, f_source);
    setup_update(&s, f_header, f_source);
    setup_delete(&s, f_header, f_source);
    setup_free(&s, f_header, f_source);
    
    fprintf(f_header, "#endif // CORM_GENERATED_H");

    fclose(f_header);
    fclose(f_source);

    LOG(INFO, "corm setup done");

    return 0;
}