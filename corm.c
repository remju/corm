#include "corm.h"
#define STB__clex_discard_preprocessor
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

int setup_reader(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating reader function for struct: %s", s->name);

    fprintf(f_header, "void read_%s(Jacon_content *db, %s *%s, const char *%s_key);\n\n", s->name, s->name, s->name, s->name);
    fprintf(f_source, "void read_%s(Jacon_content *db, %s *%s, const char *%s_key)\n{\n", s->name, s->name, s->name, s->name);
    fprintf(f_source, "    char key[128];\n");

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;

        LOG(INFO, "    adding code for field: %-10s of type: %d", field_name, field_type);
        
        // TODO: Introduce a tmp string making method to avoid using the next line for every fields
        fprintf(f_source, "    sprintf(key, \"%%s.%s\", User_key);\n", field_name);
        if (field_type & TYPE_CHAR && field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    Jacon_get_string_by_name(db, key, &%s->%s);\n", s->name, field_name);
        }
        else if (field_type & TYPE_INT)
        {
            fprintf(f_source, "    Jacon_get_int_by_name(db, key, &%s->%s);\n", s->name, field_name);
        }
        else if (field_type & TYPE_FLOAT)
        {
            fprintf(f_source, "    Jacon_get_float_by_name(db, key, &%s->%s);\n", s->name, field_name);
        }
        else if (field_type & TYPE_DOUBLE)
        {
            fprintf(f_source, "    Jacon_get_double_by_name(db, key, &%s->%s);\n", s->name, field_name);
        }
        else if (field_type & TYPE_BOOL)
        {
            fprintf(f_source, "    Jacon_get_bool_by_name(db, key, &%s->%s);\n", s->name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");

    LOG(INFO, "Reader function done for struct: %s", s->name);
    return 0;
}

int setup_free(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating free function for struct: %s", s->name);

    fprintf(f_header, "void free_%s(%s *%s);\n\n", s->name, s->name, s->name);
    fprintf(f_source, "void free_%s(%s *%s)\n{\n", s->name, s->name, s->name);

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;

        if (field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    free(%s->%s);\n", s->name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");

    LOG(INFO, "Free function done for struct: %s", s->name);
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

    FILE* f_header = fopen("example/corm_generated.h", "wb");
    if (!f_header) return 1;
    FILE* f_source = fopen("example/corm_generated.c", "wb");
    if (!f_source) return 1;

    fprintf(f_header, "#ifndef CORM_GENERATED_H\n#define CORM_GENERATED_H\n\n");
    fprintf(f_header, "#include <stdbool.h>\n#include \"../Jacon/jacon.h\"\n\n");
    fprintf(f_header, "%s\n\n", file_content);
    fprintf(f_source, "#include \"corm_generated.h\"\n#include <stdio.h>\n#include <stdlib.h>\n\n");

    setup_reader(&s, f_header, f_source);
    setup_free(&s, f_header, f_source);
    
    fprintf(f_header, "#endif // CORM_GENERATED_H");

    fclose(f_header);
    fclose(f_source);

    LOG(INFO, "corm setup done");

    return 0;
}