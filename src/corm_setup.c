#include "corm_setup_utils.h"
#include "corm_setup_types.h"
#include "corm_setup_global.h"
#include "corm_setup_json.h"
#include "corm_setup.h"
#define STB__clex_discard_preprocessor
#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#include <string.h>
#include <ctype.h>

int fill_file_content(CormContext *context)
{
    FILE *file = fopen(context->input_header, "r");
    if (!file) return 1;
    
    size_t file_size = 0;

    if (fseek(file, 0, SEEK_END) < 0) return 1;
    long ft = ftell(file);
    if (ft < 0) return 1;
    if (fseek(file, 0, SEEK_SET) < 0) return 1;
    
    file_size = ft + 1;
    char *file_content = calloc(1, file_size);
    if(file_content == NULL) return 1;

    if (fread(file_content, 1, ft, file) != (size_t)ft) return 1;
    
    context->input_file_content = file_content;
    context->input_file_size = file_size;
    return 0;
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

// Parse a field from a struct definition
// Expects the lexer to start on an identifier (type keyword)
int parse_field(Field **field, stb_lexer *lexer)
{
    int parsed_type;
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

int parse_struct(ParsedStruct *s, CormContext *context)
{
    stb_lexer lexer = {0};
    static char string_store[1024];
    stb_c_lexer_init(&lexer, context->input_file_content, context->input_file_content + context->input_file_size, string_store, sizeof(string_store));

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

int check_context(CormContext *context)
{
    int err = 0;
    if (!context->input_header)
    {
        LOG(ERROR, "Please provide a file to parse in 'context.input_header'");
        err = 1;
    }
    if (!context->crud)
    {
        LOG(ERROR, "Please set 'context.crud' according to the wanted functions");
        err = 1;
    }
    if (!context->db_type)
    {
        LOG(ERROR, "Please set 'context.db_type' according to your type of database");
        err = 1;
    }
    return err;
}

int prepare_context(CormContext *context)
{
    if (!context->output_path) context->output_path = "./";
    fill_file_content(context);
    return 0;
}

int corm_setup(CormContext context)
{
    LOG(INFO, "Starting corm setup");
    
    prepare_context(&context);
    
    ParsedStruct s = {0};
    parse_struct(&s, &context);

    FILE* f_header = fopen(tmp_str("%scorm_%s.h", context.output_path, s.lower_name), "w");
    if (!f_header) return 1;
    FILE* f_source = fopen(tmp_str("%scorm_%s.c", context.output_path, s.lower_name), "w");
    if (!f_source) return 1;
    
    fprintf(f_header, "#ifndef CORM_%s_H\n#define CORM_%s_H\n\n", s.lower_name, s.lower_name);
    fprintf(f_header, "#include \"corm.h\"\n");
    fprintf(f_source, "#include \"corm_%s.h\"\n#include <stdlib.h>\n\n", s.lower_name);
    if (context.options & DEBUG)
        fprintf(f_source, "#include <stdio.h>\n\n");
    
    switch (context.db_type)
    {
    case JSON_DATABASE:
        setup_for_json_db(&context, &s, f_header, f_source);
        break;
    }
    setup_free(&s, f_header, f_source);
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