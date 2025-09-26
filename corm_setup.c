#include "corm_setup.h"
#define STB__clex_discard_preprocessor
#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define LOG(lvl, fmt, ...) (printf("[%s] " fmt "\n", #lvl, ##__VA_ARGS__))

#define TYPE_CONST      0x01
#define TYPE_UNSIGNED   0x02
#define TYPE_CHAR       0x04
#define TYPE_INT        0x08
#define TYPE_FLOAT      0x10
#define TYPE_DOUBLE     0x20
#define TYPE_BOOL       0x40
#define TYPE_POINTER    0x80

typedef struct {
    int type;
    char *name;
} Field;

typedef struct {
    char *name;             // Struct name with 1st char upper
    size_t name_len;        // Struct name with all chars lower, used for variable names
    char *lower_name;       // Name length
    Field **fields;
    size_t field_count;
    size_t field_capacity;
} ParsedStruct;

char* read_file(FILE *file, size_t *file_size)
{
    if (fseek(file, 0, SEEK_END) < 0) return NULL;
    *file_size = ftell(file);
    if (*file_size < 0) return NULL;
    if (fseek(file, 0, SEEK_SET) < 0) return NULL;
    
    (*file_size)++; // Extra null byte
    char *file_content = calloc(1, *file_size);
    if(file_content == NULL) return NULL;

    fread(file_content, *file_size, 1, file);
    return file_content;
}

const char* tmp_str(const char *fmt, ...)
{
    static char tmp_str_buf[TMP_STR_SIZE];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp_str_buf, fmt, args);
    va_end(args);
    return tmp_str_buf;
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

    // Small utility to make code clean later
    s->name[0] = toupper(s->name[0]);           // Enforce first char to upper
    s->name_len = strlen(s->name);              
    s->lower_name = calloc(1, s->name_len);
    for (size_t i = 0; i < s->name_len; i++)    // Keep name as lower chars
    {
        s->lower_name[i] = tolower(s->name[i]);
    }

    return 0;
}

int setup_create_data_node(ParsedStruct *s, FILE *f_source)
{
    LOG(INFO, "Generating create data node function for struct: %s", s->name);

    fprintf(f_source, "Jacon_Node* create_%s_node(%s *%s, const char *key)\n{\n", s->lower_name, s->name, s->lower_name);
    
    fprintf(f_source, "    Jacon_Node *node = calloc(1, sizeof(Jacon_Node));\n");
    fprintf(f_source, "    node->name = strdup(key);\n");
    fprintf(f_source, "    node->type = JACON_VALUE_OBJECT;\n");
    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;
        fprintf(f_source, "    Jacon_Node *%s = calloc(1, sizeof(Jacon_Node));\n", field_name);
        fprintf(f_source, "    %s->name = strdup(\"%s\");\n", field_name, field_name);
        if (field_type & TYPE_CHAR && field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_STRING;\n", field_name);
            fprintf(f_source, "    %s->value.string_val = strdup(%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_INT)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_INT;\n", field_name);
            fprintf(f_source, "    %s->value.int_val = %s->%s;\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_FLOAT)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_FLOAT;\n", field_name);
            fprintf(f_source, "    %s->value.float_val = %s->%s;\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_DOUBLE)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_DOUBLE;\n", field_name);
            fprintf(f_source, "    %s->value.double_val = %s->%s;\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_BOOL)
        {
            fprintf(f_source, "    %s->type = JACON_VALUE_BOOLEAN;\n", field_name);
            fprintf(f_source, "    %s->value.bool_val = %s->%s;\n", field_name, s->lower_name, field_name);
        }
        fprintf(f_source, "    Jacon_append_child(node, %s);\n", field_name);
    }
    fprintf(f_source, "    return node;\n}\n\n");
}

int setup_read(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating read function for struct: %s", s->name);

    fprintf(f_header, "void read_%s(Jacon_content *db, %s *%s, const char *key);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void read_%s(Jacon_content *db, %s *%s, const char *key)\n{\n", s->lower_name, s->name, s->lower_name);

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;    
        if (field_type & TYPE_CHAR && field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    Jacon_get_string_by_name(db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_INT)
        {
            fprintf(f_source, "    Jacon_get_int_by_name(db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_FLOAT)
        {
            fprintf(f_source, "    Jacon_get_float_by_name(db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_DOUBLE)
        {
            fprintf(f_source, "    Jacon_get_double_by_name(db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_BOOL)
        {
            fprintf(f_source, "    Jacon_get_bool_by_name(db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_create(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating create function for struct: %s", s->name);

    fprintf(f_header, "void create_%s(Jacon_content *db, %s *%s, const char *key);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void create_%s(Jacon_content *db, %s *%s, const char *key)\n{\n", s->lower_name, s->name, s->lower_name);

    fprintf(f_source, "    Jacon_Node *node = create_%s_node(%s, key);\n", s->lower_name, s->lower_name);

    fprintf(f_source, "    Jacon_append_child(db->root, node);\n");
    fprintf(f_source, "    Jacon_add_node_to_map(&db->entries, node, NULL);\n}\n\n");
    return 0;
}

int setup_update(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating update function for struct: %s", s->name);

    fprintf(f_header, "void update_%s(Jacon_content *db, %s *%s, const char *key);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void update_%s(Jacon_content *db, %s *%s, const char *key)\n{\n", s->lower_name, s->name, s->lower_name);

    // Working with json, we do not need to care about SQL keys relations (PK,FK).
    // With SQL it would be needed to overwrite existing data, here delete and create is fine.
    // This introduces the following thing to work on.
    // TODO: Abstract the underlying storage system from CRUD functions
    //       This function should be the same for SQL, json, ...
    //       Maybe it is the time to add a context to our lib.
    //       Either have the storage defined at compile time or leave the
    //       choice to the user to define and use multiple ones at runtime.

    fprintf(f_source, "    Jacon_Node *new = create_%s_node(%s, key);\n", s->lower_name, s->lower_name);
    fprintf(f_source, "    Jacon_replace_child(db->root, key, new);\n");
    fprintf(f_source, "    Jacon_add_node_to_map(&db->entries, new, NULL);\n");

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_delete(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating delete function for struct: %s", s->name);

    fprintf(f_header, "void delete_%s(Jacon_content *db, const char *key);\n\n", s->lower_name);
    fprintf(f_source, "void delete_%s(Jacon_content *db, const char *key)\n{\n", s->lower_name);

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        fprintf(f_source, "    Jacon_free_node(Jacon_hm_remove(&db->entries, Jacon_tmp_str(\"%%s.%s\", key)));\n", field_name);
    }
    fprintf(f_source, "    Jacon_remove_child_by_name(db->root, key);\n");

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_free(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating free function for struct: %s", s->name);

    fprintf(f_header, "void free_%s(%s *%s);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void free_%s(%s *%s)\n{\n", s->lower_name, s->name, s->lower_name);

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;

        if (field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    free(%s->%s);\n", s->lower_name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_print(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating debug print function for struct: %s", s->name);

    fprintf(f_header, "#define print_%s(%s) (print_%s_(%s, #%s))\n", s->lower_name, s->lower_name, s->lower_name, s->lower_name, s->lower_name);
    fprintf(f_header, "void print_%s_(%s %s, const char *name);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void print_%s_(%s %s, const char *name)\n{\n", s->lower_name, s->name, s->lower_name);


    fprintf(f_source, "    printf(\"[DEBUG] Printing %%s\\n\", name);\n");
    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;
        if (field_type & TYPE_CHAR && field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    printf(\"    %-15s: %%s\\n\", %s.%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_INT)
        {
            fprintf(f_source, "    printf(\"    %-15s: %%d\\n\", %s.%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_FLOAT)
        {
            fprintf(f_source, "    printf(\"    %-15s: %%f\\n\", %s.%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_DOUBLE)
        {
            fprintf(f_source, "    printf(\"    %-15s: %%f\\n\", %s.%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_BOOL)
        {
            fprintf(f_source, "    printf(\"    %-15s: %%s\\n\", %s.%s ? \"true\" : \"false\");\n", field_name, s->lower_name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");
    return 0;
}

int corm_setup_(CormContext context)
{
    int err = 0;
    if (!context.input_header)
    {
        LOG(ERROR, "Please provide a file to parse in 'context.input_header'");
        err = 1;
    }
    if (!context.crud)
    {
        LOG(ERROR, "Please set 'context.crud' according to the wanted functions");
        err = 1;
    }
    if (err) return 1;

    if (!context.output_path) context.output_path = "./";

    LOG(INFO, "Starting corm setup");
    FILE *f = fopen(context.input_header, "rb");
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
    
    FILE* f_header = fopen(tmp_str("%scorm_%s.h", context.output_path, s.lower_name), "wb");
    if (!f_header) return 1;
    FILE* f_source = fopen(tmp_str("%scorm_%s.c", context.output_path, s.lower_name), "wb");
    if (!f_source) return 1;
    
    fprintf(f_header, "#ifndef CORM_%s_H\n#define CORM_%s_H\n\n", s.lower_name, s.lower_name);
    fprintf(f_header, "#include \"../Jacon/jacon.h\"\n");
    fprintf(f_header, "%s\n\n", file_content);
    fprintf(f_source, "#include \"corm_%s.h\"\n#include <stdio.h>\n#include <stdlib.h>\n\n", s.lower_name);
    
    
    if (context.crud & READ)
    {
        setup_read(&s, f_header, f_source);
    }
    if (context.crud & CREATE || context.crud & UPDATE)
    {
        setup_create_data_node(&s, f_source);
        setup_create(&s, f_header, f_source);
        setup_update(&s, f_header, f_source);
    }
    if (context.crud & DELETE)
    {
        setup_delete(&s, f_header, f_source);
    }
    if (context.crud & CREATE || context.crud & UPDATE)
    {
        setup_free(&s, f_header, f_source);
    }
    
    if (context.options & DEBUG)
    {
        setup_print(&s, f_header, f_source);
    }
    
    fprintf(f_header, "#endif // CORM_%s_H", s.lower_name);

    fclose(f_header);
    fclose(f_source);

    LOG(INFO, "corm setup done");

    return 0;
}