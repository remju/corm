#include "corm_setup_utils.h"
#include "corm_setup_global.h"


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
