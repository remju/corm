#include "corm_setup_utils.h"
#include "corm_setup_json.h"

int setup_json_create_data_node(ParsedStruct *s, FILE *f_source)
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
    return 0;
}

int setup_json_read(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating read function for struct: %s", s->name);

    fprintf(f_header, "void read_%s(CormDatabase *db, %s *%s, const char *key);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void read_%s(CormDatabase *db, %s *%s, const char *key)\n{\n", s->lower_name, s->name, s->lower_name);

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        int field_type = s->fields[i]->type;    
        if (field_type & TYPE_CHAR && field_type & TYPE_POINTER)
        {
            fprintf(f_source, "    Jacon_get_string_by_name(&db->json.db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_INT)
        {
            fprintf(f_source, "    Jacon_get_int_by_name(&db->json.db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_FLOAT)
        {
            fprintf(f_source, "    Jacon_get_float_by_name(&db->json.db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_DOUBLE)
        {
            fprintf(f_source, "    Jacon_get_double_by_name(&db->json.db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
        else if (field_type & TYPE_BOOL)
        {
            fprintf(f_source, "    Jacon_get_bool_by_name(&db->json.db, Jacon_tmp_str(\"%%s.%s\", key), &%s->%s);\n", field_name, s->lower_name, field_name);
        }
    }

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_json_create(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating create function for struct: %s", s->name);

    fprintf(f_header, "void create_%s(CormDatabase *db, %s *%s, const char *key);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void create_%s(CormDatabase *db, %s *%s, const char *key)\n{\n", s->lower_name, s->name, s->lower_name);

    fprintf(f_source, "    Jacon_Node *node = create_%s_node(%s, key);\n", s->lower_name, s->lower_name);

    fprintf(f_source, "    Jacon_append_child(db->json.db.root, node);\n");
    fprintf(f_source, "    Jacon_add_node_to_map(&db->json.db.entries, node, NULL);\n}\n\n");
    return 0;
}

int setup_json_update(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating update function for struct: %s", s->name);

    fprintf(f_header, "void update_%s(CormDatabase *db, %s *%s, const char *key);\n\n", s->lower_name, s->name, s->lower_name);
    fprintf(f_source, "void update_%s(CormDatabase *db, %s *%s, const char *key)\n{\n", s->lower_name, s->name, s->lower_name);

    // Working with json, we do not need to care about SQL keys relations (PK,FK).
    // With SQL it would be needed to overwrite existing data, here delete and create is fine.
    // This introduces the following thing to work on.
    // TODO: Abstract the underlying storage system from CRUD functions
    //       This function should be the same for SQL, json, ...
    //       Maybe it is the time to add a context to our lib.
    //       Either have the storage defined at compile time or leave the
    //       choice to the user to define and use multiple ones at runtime.

    fprintf(f_source, "    Jacon_remove_child_by_name(db->json.db.root, key);\n");
    fprintf(f_source, "    Jacon_Node *new = create_%s_node(%s, key);\n", s->lower_name, s->lower_name);
    fprintf(f_source, "    Jacon_append_child(db->json.db.root, new);\n");
    fprintf(f_source, "    Jacon_add_node_to_map(&db->json.db.entries, new, NULL);\n");

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_json_delete(ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    LOG(INFO, "Generating delete function for struct: %s", s->name);

    fprintf(f_header, "void delete_%s(CormDatabase *db, const char *key);\n\n", s->lower_name);
    fprintf(f_source, "void delete_%s(CormDatabase *db, const char *key)\n{\n", s->lower_name);

    for (size_t i = 0; i < s->field_count; i++)
    {
        char *field_name = s->fields[i]->name;
        fprintf(f_source, "    Jacon_free_node(Jacon_hm_remove(&db->json.db.entries, Jacon_tmp_str(\"%%s.%s\", key)));\n", field_name);
    }
    fprintf(f_source, "    Jacon_remove_child_by_name(db->json.db.root, key);\n");

    fprintf(f_source, "}\n\n");
    return 0;
}

int setup_for_json_db(CormContext *context, ParsedStruct *s, FILE* f_header, FILE* f_source)
{
    fprintf(f_header, "%s\n\n", context->input_file_content);
    fprintf(f_source, "#include \"jacon.h\"\n\n");
 
    if (context->crud & READ)
    {
        setup_json_read(s, f_header, f_source);
    }
    if (context->crud & CREATE || context->crud & UPDATE)
    {
        setup_json_create_data_node(s, f_source);
        setup_json_create(s, f_header, f_source);
        setup_json_update(s, f_header, f_source);
    }
    if (context->crud & DELETE)
    {
        setup_json_delete(s, f_header, f_source);
    }
    return 0;
}