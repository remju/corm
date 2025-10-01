#include "jacon.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>

#define Jacon_str_append_null(builder, ...) Jacon_str_append(builder, __VA_ARGS__, NULL)
#define Jacon_str_append_fmt_null(builder, ...) Jacon_str_append_fmt(builder, __VA_ARGS__, NULL)
#define Jacon_defer_return(value) do { ret = (value); goto defer; } while (0)

const char* Jacon_tmp_str(const char *fmt, ...)
{
    static char tmp_str_buf[JACON_TMP_STR_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp_str_buf, fmt, args);
    va_end(args);
    return tmp_str_buf;
}

Jacon_Error 
Jacon_str_append(Jacon_StringBuilder* builder, ...) 
{
    if (builder == NULL) {
        return JACON_ERR_NULL_PARAM;
    }
    va_list args;
    va_start(args, builder);

    const char* arg = va_arg(args, const char*);
    while (arg != NULL) 
    {
        size_t size = strlen(arg);
        if (size > 0) {
            if (builder->count + size + 1 > builder->capacity)
            {
                size_t new_capacity = builder->count + size + 1;
                builder->string = realloc(builder->string, new_capacity);
                if (builder->string == NULL) return JACON_ERR_MEMORY_ALLOCATION;
                builder->capacity = new_capacity;
            }
            strncpy(builder->string + builder->count, arg, size);
            builder->count += size;
            builder->string[builder->count] = '\0';
        }
        arg = va_arg(args, const char*);
    }

    va_end(args);
    return JACON_OK;
}

Jacon_Error 
Jacon_str_append_fmt(Jacon_StringBuilder* builder, const char* fmt, ...)
{
    if (builder == NULL) {
        return JACON_ERR_NULL_PARAM;
    }
    int n;
    size_t size = 0;
    va_list args;
    va_start(args, fmt);
    n = vsnprintf(NULL, size, fmt, args);
    va_end(args);
    
    if (n < 0)
        return JACON_ERR_APPEND_FSTRING;

    size = (size_t) n + 1;
    if (builder->capacity < builder->count + size) {
        size_t new_capacity = builder->count + size;
        char* tmp = realloc(builder->string, new_capacity);
        if (tmp == NULL) {
            return JACON_ERR_MEMORY_ALLOCATION;
        }
        builder->string = tmp;
        builder->capacity = new_capacity;
    }

    va_start(args, fmt);
    vsnprintf(builder->string + builder->count, size, fmt, args);
    va_end(args);

    builder->count += size - 1;

    if (n < 0) {
        return JACON_ERR_MEMORY_ALLOCATION;
    }

    return JACON_OK;
} 

void
Jacon_str_free(Jacon_StringBuilder *builder)
{
    if (builder->string != NULL) 
    {
        free(builder->string);
        builder->string = NULL;
    }
}

/**
 * djb2 algorithm
 * source: https://www.cse.yorku.ca/~oz/hash.html
 */
unsigned long
Jacon_hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * Create a new entry with the key, value pair
 */
Jacon_HashMapEntry*
Jacon_create_mapentry(const char* key, void* value)
{
    Jacon_HashMapEntry* entry = calloc(1, sizeof(Jacon_HashMapEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->key = strdup(key);
    if (entry->key == NULL) {
        free(entry);
        return NULL;
    }
    entry->value = value;
    entry->next_entry = NULL;
    return entry;
}

Jacon_Error
Jacon_hm_resize(Jacon_HashMap* map)
{
    Jacon_HashMap tmp = {0};
    tmp.size = map->size == 0 ? 
        JACON_MAP_DEFAULT_SIZE : map->size * JACON_MAP_RESIZE_FACTOR;
    tmp.entries = calloc(tmp.size, sizeof(Jacon_HashMapEntry*));
    if(tmp.entries == NULL) {
        return JACON_ERR_MEMORY_ALLOCATION;
    }

    for(size_t i = 0; i < map->size; i++) {
        if(map->entries[i] != NULL) {
            Jacon_hm_put(&tmp, map->entries[i]->key, map->entries[i]->value);
            Jacon_HashMapEntry* entry = map->entries[i]->next_entry;
            while(entry != NULL) {
                Jacon_hm_put(&tmp, entry->key, entry->value);
                entry = entry->next_entry;
            }
        }
    }
    
    for (size_t i = 0; i < map->size; i++) {
        Jacon_HashMapEntry* entry = map->entries[i];
        while (entry != NULL) {
            Jacon_HashMapEntry* next = entry->next_entry;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(map->entries);
    map->entries = NULL;
    *map = tmp;
    return JACON_OK;
}

void*
Jacon_hm_get(Jacon_HashMap* map, const char* key)
{
    if(map == NULL) {
        return NULL;
    }

    unsigned long hashcode = Jacon_hash((unsigned char*)key);
    size_t index = hashcode % map->size;

    Jacon_HashMapEntry* current = map->entries[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next_entry;
    }

    return NULL;
}

Jacon_Error
Jacon_hm_put(Jacon_HashMap* map, const char* key, void* value)
{
    if (map == NULL || key == NULL) {
        return JACON_ERR_NULL_PARAM;
    }

    // Resize if necessary
    if (map->entries_count == map->size) {
        int ret = Jacon_hm_resize(map);
        if (ret != JACON_OK) return ret;
    }

    // Hashcode and bucket index
    unsigned long hashcode = Jacon_hash((unsigned char*)key);
    size_t index = hashcode % map->size;

    Jacon_HashMapEntry* current = map->entries[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->value != NULL) {
                Jacon_free_node(current->value);
            }
            current->value = value;
            return JACON_OK;
        }
        current = current->next_entry;
    }

    Jacon_HashMapEntry* new_entry = Jacon_create_mapentry(key, value);
    if (new_entry == NULL) return JACON_ERR_MEMORY_ALLOCATION;

    new_entry->next_entry = map->entries[index];
    map->entries[index] = new_entry;
    map->entries_count++;
    return JACON_OK;
}

void*
Jacon_hm_remove(Jacon_HashMap* map, const char* key){
    if(map == NULL) {
        return NULL;
    }

    unsigned long hashcode = Jacon_hash((unsigned char*)key);
    size_t index = hashcode % map->size;

    Jacon_HashMapEntry* current = map->entries[index];
    Jacon_HashMapEntry* prev = NULL;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            void* value = current->value;
            if (prev == NULL) {
                map->entries[index] = current->next_entry;
            } else {
                prev->next_entry = current->next_entry;
            }
            free(current->key);
            free(current);
            map->entries_count--;
            return value;
        }
        prev = current;
        current = current->next_entry;
    }

    return NULL;
}

void
Jacon_hm_free_entry(Jacon_HashMapEntry* entry)
{
    if (entry == NULL) {
        return;
    }
    if (entry->key != NULL) {
        free(entry->key);
        entry->key = NULL;
    }
    if (entry->value != NULL) {
        Jacon_free_node(entry->value);
        entry->value = NULL;
    }
    free(entry);
}

void 
Jacon_hm_free(Jacon_HashMap* map) 
{
    if (map == NULL || map->entries == NULL)
        return;
    for (size_t i = 0; i < map->size; i++) {
        Jacon_HashMapEntry* entry = map->entries[i];
        while (entry != NULL) {
            Jacon_HashMapEntry* next = entry->next_entry;
            Jacon_hm_free_entry(entry);
            entry = next;
        }
    }
    free(map->entries);
    map->entries = NULL;
}

/**
 * Create a new entry with the key
 */
Jacon_HashSetEntry*
Jacon_create_setentry(const char* key)
{
    Jacon_HashSetEntry* entry = calloc(1, sizeof(Jacon_HashSetEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->key = strdup(key);
    if (entry->key == NULL) {
        free(entry);
        return NULL;
    }
    entry->next = NULL;
    return entry;
}

Jacon_Error
Jacon_hs_resize(Jacon_HashSet* set)
{
    Jacon_HashSet tmp = {0};
    tmp.capacity = set->capacity * JACON_MAP_RESIZE_FACTOR;
    tmp.entries = calloc(tmp.capacity, sizeof(Jacon_HashSetEntry*));
    if(tmp.entries == NULL) {
        return JACON_ERR_MEMORY_ALLOCATION;
    }

    for(size_t i = 0; i < set->capacity; i++) {
        if(set->entries[i] != NULL) {
            Jacon_hs_put(&tmp, set->entries[i]->key);
            Jacon_HashSetEntry* entry = set->entries[i]->next;
            while(entry != NULL) {
                Jacon_hs_put(&tmp, entry->key);
                entry = entry->next;
            }
        }
    }
    Jacon_hs_free(set);
    *set = tmp;
    return JACON_OK;
}


bool
Jacon_hs_exists(Jacon_HashSet *set, const char* key)
{
    unsigned long hashcode = Jacon_hash((unsigned char*)key);
    size_t index = hashcode % set->capacity;

    Jacon_HashSetEntry* current = set->entries[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return true;
        }
        current = current->next;
    }

    return false;
}

Jacon_Error
Jacon_hs_put(Jacon_HashSet *set, const char* key)
{
    if(set == NULL) {
        return JACON_ERR_NULL_PARAM;
    }
    if(key == NULL) {
        return JACON_ERR_NULL_PARAM;
    }

    int ret;
    // Resize if set is full
    // Subject to change to partially full (75%)
    if(set->capacity == set->count) {
        ret = Jacon_hs_resize(set);
        if (ret != JACON_OK) return ret;
    }

    // Hashcode and bucket index
    unsigned long hashcode = Jacon_hash((unsigned char*)key);
    size_t index = hashcode % set->capacity;

    Jacon_HashSetEntry* new_entry = Jacon_create_setentry(key);
    if (new_entry == NULL) {
        return JACON_ERR_MEMORY_ALLOCATION;
    }

    if(set->entries[index] == NULL) { // Empty bucket
        set->entries[index] = new_entry;
    }
    else { // Full bucket
        Jacon_HashSetEntry* current = set->entries[index];
        while (current != NULL) {
            if (strcmp(current->key, key) == 0) {
                return JACON_OK;
            }
            current = current->next;
        }
        new_entry->next = set->entries[index];
        set->entries[index] = new_entry;
    }
    set->count++;
    return JACON_OK;
}

void
Jacon_hs_free_entry(Jacon_HashSetEntry* entry)
{
    if (entry == NULL) {
        return;
    }
    if (entry->key != NULL) {
        free(entry->key);
        entry->key = NULL;
    }
    free(entry);
}

void 
Jacon_hs_free(Jacon_HashSet* set) 
{
    if (set == NULL || set->entries == NULL)
        return;
    for (size_t i = 0; i < set->capacity; i++) {
        Jacon_HashSetEntry* entry = set->entries[i];
        while (entry != NULL) {
            Jacon_HashSetEntry* next = entry->next;
            Jacon_hs_free_entry(entry);
            entry = next;
        }
    }
    free(set->entries);
    set->entries = NULL;
}

Jacon_Error
Jacon_hs_remove(Jacon_HashSet *set, const char* key)
{
    if(set == NULL) {
        return JACON_ERR_NULL_PARAM;
    }

    unsigned long hashcode = Jacon_hash((unsigned char*)key);
    size_t index = hashcode % set->capacity;

    Jacon_HashSetEntry* current = set->entries[index];
    Jacon_HashSetEntry* prev = NULL;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev != NULL) {
                prev->next = current->next;
            }
            Jacon_hs_free_entry(current);
            set->count--;
            return JACON_OK;
        }
        prev = current;
        current = current->next;
    }

    return JACON_OK;
}

// Debug print token
void 
Jacon_print_token(const Jacon_Token* token) 
{
    switch (token->type) {
        case JACON_TOKEN_STRING:
            printf("Token Type: STRING, Value: %s\n", token->string_val);
            break;
        case JACON_TOKEN_INT:
            printf("Token Type: INT, Value: %d\n", token->int_val);
            break;
        case JACON_TOKEN_FLOAT:
            printf("Token Type: FLOAT, Value: %f\n", token->float_val);
            break;
        case JACON_TOKEN_DOUBLE:
            printf("Token Type: DOUBLE, Value: %lf\n", token->double_val);
            break;
        case JACON_TOKEN_BOOLEAN:
            printf("Token Type: BOOLEAN, Value: %s\n", token->bool_val ? "true" : "false");
            break;
        case JACON_TOKEN_ARRAY_START:
            printf("Token Type: ARRAY_START\n");
            break;
        case JACON_TOKEN_ARRAY_END:
            printf("Token Type: ARRAY_END\n");
            break;
        case JACON_TOKEN_OBJECT_START:
            printf("Token Type: OBJECT_START\n");
            break;
        case JACON_TOKEN_OBJECT_END:
            printf("Token Type: OBJECT_END\n");
            break;
        case JACON_TOKEN_NULL:
            printf("Token Type: NULL VALUE\n");
            break;
        case JACON_TOKEN_COLON:
            printf("Token Type: COLON\n");
            break;
        case JACON_TOKEN_COMMA:
            printf("Token Type: COMMA\n");
            break;
        default:
            printf("Unknown Token Type\n");
            break;
    }
}

// Debug print tokenizer
void 
Jacon_print_tokenizer(const Jacon_Tokenizer* tokenizer) 
{
    printf("Tokenizer contains %zu tokens:\n", tokenizer->count);
    for (size_t i = 0; i < tokenizer->count; ++i) {
        printf("Token %zu: ", i);
        Jacon_print_token(&tokenizer->tokens[i]);
    }
}

void 
Jacon_print_node_value(Jacon_Value value, Jacon_ValueType type) 
{
    switch (type) {
        case JACON_VALUE_STRING:
            printf("\"%s\"", value.string_val);
            break;
        case JACON_VALUE_INT:
            printf("%d", value.int_val);
            break;
        case JACON_VALUE_FLOAT:
            printf("%f", value.float_val);
            break;
        case JACON_VALUE_DOUBLE:
            printf("%lf", value.double_val);
            break;
        case JACON_VALUE_BOOLEAN:
            printf("%s", value.bool_val ? "true" : "false");
            break;
        case JACON_VALUE_NULL:
            printf("null");
            break;
        case JACON_VALUE_OBJECT:
        case JACON_VALUE_ARRAY:
            break;
    }
}

void 
Jacon_print_node(Jacon_Node* node, int indent) 
{
    // Print indentation
    for (int i = 0; i < indent; ++i) {
        printf("  ");
    }

    // Print node name
    if (node->name) {
        printf("\"%s\": ", node->name);
    }

    // Print node value
    switch (node->type) {
        case JACON_VALUE_OBJECT:
            printf("{\n");
            for (size_t i = 0; i < node->child_count; ++i) {
                Jacon_print_node(node->childs[i], indent + 1);
                if (i < node->child_count - 1) {
                    printf(",");
                }
                printf("\n");
            }
            for (int i = 0; i < indent; ++i) {
                printf("  ");
            }
            printf("}");
            break;
        case JACON_VALUE_ARRAY:
            printf("[\n");
            for (size_t i = 0; i < node->child_count; ++i) {
                Jacon_print_node(node->childs[i], indent + 1);
                if (i < node->child_count - 1) {
                    printf(",");
                }
                printf("\n");
            }
            for (int i = 0; i < indent; ++i) {
                printf("  ");
            }
            printf("]");
            break;
        case JACON_VALUE_STRING:
        case JACON_VALUE_INT:
        case JACON_VALUE_FLOAT:
        case JACON_VALUE_DOUBLE:
        case JACON_VALUE_BOOLEAN:
        case JACON_VALUE_NULL:
        default:
            Jacon_print_node_value(node->value, node->type);
            break;
    }
}

Jacon_Error
Jacon_tokenizer_init(Jacon_Tokenizer* tokenizer)
{
    tokenizer->tokens = (Jacon_Token*)calloc(
        JACON_TOKENIZER_DEFAULT_CAPACITY, sizeof(Jacon_Token));
    if (tokenizer->tokens == NULL) {
        perror("Jacon_append_token array alloc error");
        return JACON_ERR_MEMORY_ALLOCATION;
    }
    tokenizer->capacity = JACON_TOKENIZER_DEFAULT_CAPACITY;
    tokenizer->count = 0;
    return JACON_OK;
}

Jacon_Error
Jacon_init_content(Jacon_content* content)
{
    // int ret;
    content->root = (Jacon_Node*)calloc(1, sizeof(Jacon_Node));
    if (content->root == NULL) return JACON_ERR_MEMORY_ALLOCATION;
    // ret = Jacon_hm_create(&content->entries, 10);
    content->entries = (Jacon_HashMap){
        .entries = calloc(10, sizeof(Jacon_HashMapEntry*)),
        .size = 10
    };
    // if (ret != JACON_OK) return ret;
    return JACON_OK;
}

void
Jacon_free_tokenizer(Jacon_Tokenizer* tokenizer)
{
    for (size_t i = 0; i < tokenizer->count; i++)
    {
        if (tokenizer->tokens[i].type == JACON_TOKEN_STRING)
            free(tokenizer->tokens[i].string_val);
    }
    if (tokenizer->tokens) {
        free(tokenizer->tokens);
        tokenizer->tokens = NULL;
    }
}

Jacon_Node* 
Jacon_duplicate_node(const Jacon_Node* node) 
{
    if (node == NULL) {
        return NULL;
    }

    Jacon_Node* new_node = (Jacon_Node*)calloc(1, sizeof(Jacon_Node));
    if (new_node == NULL) {
        return NULL;
    }

    new_node->parent = node->parent;
    new_node->name = node->name ? strdup(node->name) : NULL;
    new_node->type = node->type;

    switch (node->type) {
        case JACON_VALUE_STRING:
            new_node->value.string_val = node->value.string_val ? strdup(node->value.string_val) : NULL;
            break;
        case JACON_VALUE_INT:
            new_node->value.int_val = node->value.int_val;
            break;
        case JACON_VALUE_FLOAT:
            new_node->value.float_val = node->value.float_val;
            break;
        case JACON_VALUE_DOUBLE:
            new_node->value.double_val = node->value.double_val;
            break;
        case JACON_VALUE_BOOLEAN:
            new_node->value.bool_val = node->value.bool_val;
            break;
        case JACON_VALUE_NULL:
            // No additional data to copy
            break;
        case JACON_VALUE_ARRAY:
        case JACON_VALUE_OBJECT:
            new_node->child_count = node->child_count;
            new_node->child_capacity = node->child_capacity;
            new_node->childs = calloc(new_node->child_capacity, sizeof(Jacon_Node*));
            if (new_node->childs == NULL) {
                free(new_node->name);
                new_node->name = NULL;
                free(new_node);
                new_node = NULL;
                return NULL;
            }
            for (size_t i = 0; i < node->child_count; i++) {
                new_node->childs[i] = Jacon_duplicate_node(node->childs[i]);
                if (new_node->childs[i] == NULL) {
                    for (size_t j = 0; j < i; j++) {
                        Jacon_free_node(new_node->childs[j]);
                    }
                    free(new_node->childs);
                    free(new_node->name);
                    free(new_node);
                    return NULL;
                }
                new_node->childs[i]->parent = new_node;
            }
            break;
    }

    return new_node;
}

void
Jacon_free_node(Jacon_Node* node)
{
    if (node->name != NULL) {
        free(node->name);
        node->name = NULL;
    }
    if (node->type == JACON_VALUE_STRING && node->value.string_val != NULL) {
        free(node->value.string_val);
        node->value.string_val = NULL;
    }
    if (node->type == JACON_VALUE_ARRAY || node->type == JACON_VALUE_OBJECT) {
        for (size_t i = 0; i < node->child_count; i++)
        {
            Jacon_free_node(node->childs[i]);
            node->childs[i] = NULL;
        }
        free(node->childs);
        node->childs = NULL;
    }
    free(node);
    node = NULL;
}

Jacon_Error
Jacon_append_token(Jacon_Tokenizer* tokenizer, Jacon_Token token)
{
    if (tokenizer == NULL) {
        return JACON_ERR_NULL_PARAM;
    }

    size_t new_count = tokenizer->count + 1;
    if (new_count > tokenizer->capacity) {
        size_t new_capacity = tokenizer->capacity == 0 ?
            JACON_TOKENIZER_DEFAULT_CAPACITY : tokenizer->capacity * 2;
        tokenizer->tokens = realloc(tokenizer->tokens, new_capacity * sizeof(Jacon_Token));
        if (!tokenizer->tokens) {
            return JACON_ERR_MEMORY_ALLOCATION;
        }
        tokenizer->capacity = new_capacity;
    }
    tokenizer->tokens[tokenizer->count++] = token;
    return JACON_OK;
}

Jacon_Error
Jacon_append_child(Jacon_Node* node, Jacon_Node* child)
{
    if (node == NULL) {
        return JACON_ERR_NULL_PARAM;
    }

    size_t new_count = node->child_count + 1;
    if (new_count > node->child_capacity) {
        size_t new_capacity = node->child_capacity == 0 ?
            JACON_NODE_DEFAULT_CHILD_CAPACITY : 
            node->child_capacity * JACON_NODE_DEFAULT_RESIZE_FACTOR;
        node->childs = realloc(node->childs, new_capacity * sizeof(Jacon_Node*));
        if (!node->childs) {
            perror("Jacon_append_node_child array alloc error");
            return JACON_ERR_MEMORY_ALLOCATION;
        }
        node->child_capacity = new_capacity;
    }
    node->childs[node->child_count++] = child;
    return JACON_OK;
}

Jacon_Error
Jacon_replace_child(Jacon_Node* parent, const char* name, Jacon_Node* new)
{
    if (parent == NULL || name == NULL || new == NULL) {
        return JACON_ERR_NULL_PARAM;
    }

    for (size_t i = 0; i < parent->child_count; i++) {
        Jacon_Node* child = parent->childs[i];

        if (child->name && strcmp(child->name, name) == 0) {
            new->parent = parent;
            parent->childs[i] = new;
            Jacon_free_node(child);
            return JACON_OK;
        }
    }
    return JACON_ERR_CHILD_NOT_FOUND;
}

Jacon_Error
Jacon_remove_child_by_name(Jacon_Node* parent, const char* name)
{
    if (parent == NULL || name == NULL) {
        return JACON_ERR_NULL_PARAM;
    }

    for (size_t i = 0; i < parent->child_count; i++) {
        Jacon_Node* child = parent->childs[i];

        if (child->name && strcmp(child->name, name) == 0) {
            Jacon_free_node(child);

            for (size_t j = i; j < parent->child_count - 1; j++) {
                parent->childs[j] = parent->childs[j + 1];
            }
            parent->child_count--;
            parent->childs[parent->child_count] = NULL;
            return JACON_OK;
        }
    }
    return JACON_ERR_CHILD_NOT_FOUND;
}

Jacon_Node*
Jacon_get_child_by_name(Jacon_Node *parent, const char *name)
{
    if (parent == NULL || name == NULL) return NULL;
    for (size_t i = 0; i < parent->child_count; i++) {
        Jacon_Node *child = parent->childs[i];
        if (strcmp(child->name, name) == 0) {
            return child;
        }
    }
    return NULL;
}

// Check if is a valid hex char
bool 
Jacon_is_hex_digit(char c)
{
    return isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f');
}

// Only allow json allowed whitespace chars
bool
Jacon_is_whitespace(char c)
{
    return c == ' ' || c == '\r' || c == '\t' || c == '\n';
}

Jacon_Error
Jacon_validate_string(const char* str)
{
    const char *ptr = str;
    while (*ptr) {
        if (*ptr == '\n'
            || *ptr == '\t') return JACON_ERR_INVALID_ESCAPE_SEQUENCE;
        else if (*ptr == '\\') {
            ptr++;
            if (*ptr == 'u') {
                for (int i = 1; i < 5; ++i) {
                    if (!Jacon_is_hex_digit(*(ptr + i))) {
                        return JACON_ERR_INVALID_ESCAPE_SEQUENCE;
                    }
                }
                ptr += 4; // Skip valid escape sequence
            } else if (*ptr == '"' 
                || *ptr == '\\' 
                || *ptr == '/' 
                || *ptr == 'b' 
                || *ptr == 'f' 
                || *ptr == 'n' 
                || *ptr == 'r' 
                || *ptr == 't') {
                ptr++;
            } else {
                return JACON_ERR_INVALID_ESCAPE_SEQUENCE; // Invalid escape sequence
            }
        } else {
            ptr++; // Move to the next character
        }
    }
    return JACON_OK;
}

Jacon_Error 
Jacon_parse_token(Jacon_Token* token, const char** str) 
{
    switch (**str) {
        case ',':
            token->type = JACON_TOKEN_COMMA;
            (*str)++;
            break;
        case ':':
            token->type = JACON_TOKEN_COLON;
            (*str)++;
            break;
        case '{':
            token->type = JACON_TOKEN_OBJECT_START;
            (*str)++;
            break;
        case '}':
            token->type = JACON_TOKEN_OBJECT_END;
            (*str)++;
            break;
        case '[':
            token->type = JACON_TOKEN_ARRAY_START;
            (*str)++;
            break;
        case ']':
            token->type = JACON_TOKEN_ARRAY_END;
            (*str)++;
            break;
        case '"':
            (*str)++; // Move past the initial quote
            const char* string_end = strchr(*str, '"');
            if (string_end == NULL) return JACON_ERR_CHAR_NOT_FOUND;

            // While the double quote is escaped, find the next one
            while (string_end[-1] == '\\') {
                string_end = strchr(string_end + 1, '"');
                if (string_end == NULL) return JACON_ERR_CHAR_NOT_FOUND;
            }
            token->type = JACON_TOKEN_STRING;
            size_t string_size = string_end - *str;
            token->string_val = (char*)calloc(string_size + 1, sizeof(char));
            if (token->string_val == NULL) return JACON_ERR_MEMORY_ALLOCATION;

            strncpy(token->string_val, *str, string_size);
            token->string_val[string_size] = '\0';

            int ret = Jacon_validate_string(token->string_val);
            if (ret != JACON_OK) {
                free(token->string_val);
                return ret;
            }

            *str = string_end + 1; // Move past the closing quote
            break;
        case('n'):
            if (strncmp(*str, "null", 4) != 0) return JACON_ERR_INVALID_JSON;
            token->type = JACON_TOKEN_NULL;
            (*str) += 4;
            break;
        case('t'):
            if (strncmp(*str, "true", 4) != 0) return JACON_ERR_INVALID_JSON;
            token->type = JACON_TOKEN_BOOLEAN;
            token->bool_val = true;
            (*str) += 4;
            break;
        case('f'):
            if (strncmp(*str, "false", 5) != 0) return JACON_ERR_INVALID_JSON;
            token->type = JACON_TOKEN_BOOLEAN;
            token->bool_val = false;
            (*str) += 5;
            break;
        case('\0'):
            return JACON_END_OF_INPUT;
        default:
            // Check if whitespace
            if(Jacon_is_whitespace(**str)) {
                (*str)++;
                return Jacon_parse_token(token, str);
            }
            // Invalidate hex values
            else if (**str == '0' && (*str)[1] == 'x') return JACON_ERR_INVALID_JSON;
            // Invalidate leading 0
            else if (**str == '0' && (*str)[1] >= '0' && (*str)[1] <= '9') return JACON_ERR_INVALID_JSON;
            // Check if char is a number or a negative number
            else if ((**str >= '0' && **str <= '9') 
                || (**str == '-' && (((*str)[1] >= '0' && (*str)[1] <= '9')))) {
                if (**str == '-' && (*str)[1] == '0' && ((*str)[2] != '.' || !isdigit((*str)[2]))) {
                    const char* p = *str + 1;
                    while (*p == '0') p++;
                    if (isdigit(*p)) return JACON_ERR_INVALID_JSON;
                }
                char* dot = strchr(*str, '.');
                if (dot) {
                    // Check for leading 0, if real, check if distance between last '0' and '.' > 1
                    char* last_zero = strrchr(*str, '0');
                    if (last_zero && dot - last_zero > 1) return JACON_ERR_INVALID_JSON;
                    // Check if there is a dot and if so if there are numbers after
                    char* after_dot = dot + 1;
                    if (!isdigit(*after_dot)) return JACON_ERR_INVALID_JSON;
                }
                
                char *endptr;
                // Parse an int
                int ival = (int)strtol(*str, &endptr, 10);
                if ((isspace(*endptr) || *endptr == ',' || *endptr == ']' || *endptr == '}' || *endptr == '\0')
                    && ival >= INT_MIN && ival <= INT_MAX) {
                    token->int_val = ival;
                    token->type = JACON_TOKEN_INT;
                    *str = endptr;
                    break;
                }

                double dval = strtod(*str, &endptr);
    
                // Ensure the number is followed by valid JSON characters
                if (!isspace(*endptr) && *endptr != ',' && *endptr != ']' && *endptr != '}' && *endptr != '\0') {
                    return JACON_ERR_INVALID_JSON;
                }
                
                float fval = (float)dval;
                double recast = (double)fval;
                if (dval > recast) {
                    token->type = JACON_TOKEN_DOUBLE;
                    token->double_val = dval;
                } else {
                    token->type = JACON_TOKEN_FLOAT;
                    token->float_val = fval;
                }

                *str = endptr;
            }
            else return JACON_ERR_INVALID_JSON;
    }
    return JACON_OK;
}

Jacon_Error 
Jacon_tokenize(Jacon_Tokenizer* tokenizer, const char* str) 
{
    Jacon_Error ret;

    while (*str) {
        Jacon_Token token = {0};
        ret = Jacon_parse_token(&token, &str);
        if (ret == JACON_END_OF_INPUT) return JACON_OK;
        if (ret != JACON_OK) return ret;
        ret = Jacon_append_token(tokenizer, token);
        if (ret != JACON_OK) return ret;
    }

    return JACON_OK;
}

Jacon_Error
Jacon_current_token(Jacon_Token* token, Jacon_Tokenizer* tokenizer, size_t current_index)
{
    if (current_index >= tokenizer->count) return JACON_ERR_INDEX_OUT_OF_BOUND;
    *token = tokenizer->tokens[current_index];
    return JACON_OK;
}

Jacon_Error
Jacon_consume_token(Jacon_Token* token, Jacon_Tokenizer* tokenizer, size_t* current_index)
{
    if (*current_index >= tokenizer->count) return JACON_ERR_INDEX_OUT_OF_BOUND;
    *token = tokenizer->tokens[*current_index + 1];
    *current_index += 1;
    return JACON_OK;
}

Jacon_Error
Jacon_validate_object(Jacon_Tokenizer* tokenizer, size_t* index);
Jacon_Error
Jacon_validate_array(Jacon_Tokenizer* tokenizer, size_t* index);

Jacon_Error
Jacon_validate_array(Jacon_Tokenizer* tokenizer, size_t* index)
{
    int ret = JACON_OK;
    Jacon_Token* current = NULL;
    bool last_value = false;
    
    if (*index < tokenizer->count) {
        current = &tokenizer->tokens[*index];
    } else {
        return JACON_ERR_INVALID_JSON;
    }

    while (current->type != JACON_TOKEN_ARRAY_END) {
        current = &tokenizer->tokens[*index];
        switch (current->type) {
            case JACON_TOKEN_ARRAY_START:
                (*index)++;
                ret = Jacon_validate_array(tokenizer, index);
                if (ret != JACON_OK) return ret;
                last_value = true;
                break;
            case JACON_TOKEN_OBJECT_START:
                (*index)++;
                ret = Jacon_validate_object(tokenizer, index);
                if (ret != JACON_OK) return ret;
                last_value = true;
                break;
            case JACON_TOKEN_COMMA:
                if (!last_value) {
                    return JACON_ERR_INVALID_JSON;
                }
                else {
                    last_value = false;
                    (*index)++;    
                }
                break;
            case JACON_TOKEN_ARRAY_END:
                return JACON_ERR_UNREACHABLE_STATEMENT;
            case JACON_TOKEN_OBJECT_END:
            case JACON_TOKEN_COLON:
                return JACON_ERR_INVALID_JSON;
            case JACON_TOKEN_STRING:
            case JACON_TOKEN_INT:
            case JACON_TOKEN_DOUBLE:
            case JACON_TOKEN_FLOAT:
            case JACON_TOKEN_BOOLEAN:
            case JACON_TOKEN_NULL:
            default:
                if(last_value) return JACON_ERR_INVALID_JSON;
                last_value = true;
                (*index)++;
                break;
        }
        if (*index >= tokenizer->count) {
            return JACON_ERR_INVALID_JSON;
        }
        current = &tokenizer->tokens[*index];
    }
    (*index)++;
    return JACON_OK;
}

Jacon_Error
Jacon_validate_object(Jacon_Tokenizer* tokenizer, size_t* index)
{
    int ret = JACON_OK;
    if (*index >= tokenizer->count) return JACON_ERR_INVALID_JSON;
    if (*index >= tokenizer->count) {
        return JACON_ERR_INVALID_JSON;
    }
    Jacon_Token* current = &tokenizer->tokens[*index];
    Jacon_Token* last = NULL;
    Jacon_HashSet names_set = (Jacon_HashSet){
        .entries = calloc(10, sizeof(Jacon_HashSetEntry*)),
        .capacity = 10
    };
    if (names_set.entries == NULL) {
        Jacon_defer_return(JACON_ERR_MEMORY_ALLOCATION);
    }
    bool last_value = false;
    while (current->type != JACON_TOKEN_OBJECT_END) {
        current = &tokenizer->tokens[*index];
        switch (current->type) {
            case JACON_TOKEN_ARRAY_START:
                if (last == NULL || last->type != JACON_TOKEN_COLON)
                    Jacon_defer_return(JACON_ERR_INVALID_JSON);

                (*index)++;
                ret = Jacon_validate_array(tokenizer, index);
                if (ret != JACON_OK) Jacon_defer_return(ret);
                last_value = true;
                break;
            case JACON_TOKEN_OBJECT_START:
                if (last == NULL || last->type != JACON_TOKEN_COLON)
                    Jacon_defer_return(JACON_ERR_INVALID_JSON);

                (*index)++;
                ret = Jacon_validate_object(tokenizer, index);
                if (ret != JACON_OK) Jacon_defer_return(ret);
                last_value = true;
                break;
            case JACON_TOKEN_COMMA:
                if (last == NULL) Jacon_defer_return(JACON_ERR_INVALID_JSON);
                else if (!last_value) {
                    Jacon_defer_return(JACON_ERR_INVALID_JSON);
                }
                last = &tokenizer->tokens[*index];
                (*index)++;
                last_value = false;
                break;
            case JACON_TOKEN_ARRAY_END:
                Jacon_defer_return(JACON_ERR_INVALID_JSON);
            case JACON_TOKEN_OBJECT_END:
                Jacon_defer_return(JACON_ERR_UNREACHABLE_STATEMENT);
            case JACON_TOKEN_COLON:
                if (last == NULL) Jacon_defer_return(JACON_ERR_INVALID_JSON);
                if (last->type != JACON_TOKEN_STRING) Jacon_defer_return(JACON_ERR_INVALID_JSON);
                last = &tokenizer->tokens[*index];
                (*index)++;
                break;
            case JACON_TOKEN_STRING:
                if (last == NULL) {
                    if (Jacon_hs_exists(&names_set, tokenizer->tokens[*index].string_val)) {
                        Jacon_defer_return(JACON_ERR_DUPLICATE_NAME);
                    }
                    Jacon_hs_put(&names_set, tokenizer->tokens[*index].string_val);
                    last = &tokenizer->tokens[*index];
                    (*index)++;
                }
                else if (last->type == JACON_TOKEN_COMMA) {
                    if (Jacon_hs_exists(&names_set, tokenizer->tokens[*index].string_val)) {
                        Jacon_defer_return(JACON_ERR_DUPLICATE_NAME);
                    }
                    Jacon_hs_put(&names_set, tokenizer->tokens[*index].string_val);
                    last = &tokenizer->tokens[*index];
                    (*index)++;
                    last_value = true;
                }
                else if (last->type == JACON_TOKEN_COLON ) {
                    last = &tokenizer->tokens[*index];
                    (*index)++;
                    last_value = true;
                }
                else {
                    Jacon_defer_return(JACON_ERR_UNREACHABLE_STATEMENT);
                }
                break;
            case JACON_TOKEN_INT:
            case JACON_TOKEN_DOUBLE:
            case JACON_TOKEN_FLOAT:
            case JACON_TOKEN_BOOLEAN:
            case JACON_TOKEN_NULL:
                if (last == NULL || last->type != JACON_TOKEN_COLON)
                    Jacon_defer_return(JACON_ERR_INVALID_JSON);
                last = &tokenizer->tokens[*index];
                (*index)++;
                last_value = true;
                break;
            default:
                Jacon_defer_return(JACON_ERR_UNREACHABLE_STATEMENT);
        }
        if (*index >= tokenizer->count) {
            Jacon_defer_return(JACON_ERR_INVALID_JSON);
        }
        current = &tokenizer->tokens[*index];
    }
    (*index)++;
defer:
    Jacon_hs_free(&names_set);
    return JACON_OK;
}

Jacon_Error
Jacon_validate_value(Jacon_Token token)
{
    switch (token.type) {
        case JACON_TOKEN_STRING:
        case JACON_TOKEN_INT:
        case JACON_TOKEN_DOUBLE:
        case JACON_TOKEN_FLOAT:
        case JACON_TOKEN_BOOLEAN:
        case JACON_TOKEN_NULL:
            return JACON_OK;
        case JACON_TOKEN_ARRAY_START:
        case JACON_TOKEN_ARRAY_END:
        case JACON_TOKEN_OBJECT_START:
        case JACON_TOKEN_OBJECT_END:
        case JACON_TOKEN_COMMA:
        case JACON_TOKEN_COLON:
            return JACON_ERR_INVALID_JSON;
    }
    return JACON_ERR_UNREACHABLE_STATEMENT;
}

/**
 * Validate a Jacon tokenizer result
 */
Jacon_Error
Jacon_validate_input(Jacon_Tokenizer* tokenizer)
{
    int ret = JACON_OK;

    if (tokenizer->count == 1) {
        ret = Jacon_validate_value(tokenizer->tokens[0]);
        return ret;
    }

    Jacon_Token* current = NULL;
    size_t index = 0;
    current = &tokenizer->tokens[index];
    switch (current->type) {
        case JACON_TOKEN_ARRAY_START:
            index++;
            ret = Jacon_validate_array(tokenizer, &index);
            if (ret != JACON_OK) return ret;
            break;
        case JACON_TOKEN_OBJECT_START:
            index++;
            ret = Jacon_validate_object(tokenizer, &index);
            if (ret != JACON_OK) return ret;
            break;
        case JACON_TOKEN_STRING:
        case JACON_TOKEN_INT:
        case JACON_TOKEN_DOUBLE:
        case JACON_TOKEN_FLOAT:
        case JACON_TOKEN_BOOLEAN:
        case JACON_TOKEN_NULL:
        case JACON_TOKEN_ARRAY_END:
        case JACON_TOKEN_OBJECT_END:
        case JACON_TOKEN_COMMA:
        case JACON_TOKEN_COLON:
        default:
            return JACON_ERR_UNREACHABLE_STATEMENT;
    }
    if (index < tokenizer->count) return JACON_ERR_INVALID_JSON;

    return JACON_OK;
}

Jacon_Error
Jacon_parse_node(Jacon_Node* node, Jacon_Tokenizer* tokenizer, size_t* current_index)
{
    int ret = JACON_OK;
    Jacon_Token current_token;
    ret = Jacon_current_token(&current_token, tokenizer, *current_index);
    if (ret != JACON_OK) return ret;
    switch (current_token.type) {
        case JACON_TOKEN_OBJECT_START:
            node->type = JACON_VALUE_OBJECT;

            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            while (current_token.type != JACON_TOKEN_OBJECT_END) {
                Jacon_Node* child = calloc(1, sizeof(Jacon_Node));
                if (child == NULL) return JACON_ERR_MEMORY_ALLOCATION;
                child->parent = node;
                ret = Jacon_parse_node(child, tokenizer, current_index);
                if (ret != JACON_OK) {
                    Jacon_free_node(child);
                    return ret;
                }

                ret = Jacon_append_child(node, child);
                if (ret != JACON_OK) return ret;
                ret = Jacon_current_token(&current_token, tokenizer, *current_index);
                if (ret != JACON_OK) return ret;
            }
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            break;

        case JACON_TOKEN_ARRAY_START:
            node->type = JACON_VALUE_ARRAY;
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            while (current_token.type != JACON_TOKEN_ARRAY_END) {
                Jacon_Node* child = calloc(1, sizeof(Jacon_Node));
                if (child == NULL) return JACON_ERR_MEMORY_ALLOCATION;
                child->parent = node;
                ret = Jacon_parse_node(child, tokenizer, current_index);
                if (ret != JACON_OK) {
                    Jacon_free_node(child);
                    return ret;
                }

                ret = Jacon_append_child(node, child);
                if (ret != JACON_OK) return ret;
                ret = Jacon_current_token(&current_token, tokenizer, *current_index);
                if (ret != JACON_OK) return ret;
            }
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            break;

        case JACON_TOKEN_STRING:
            if (node->type == JACON_VALUE_STRING) {
                node->value.string_val = strdup(current_token.string_val);
                if (node->value.string_val == NULL) return JACON_ERR_MEMORY_ALLOCATION;

                ret = Jacon_consume_token(&current_token, tokenizer, current_index);
                if (ret != JACON_OK) return ret;
            }
            else if (node->parent != NULL && node->parent->type == JACON_VALUE_ARRAY) {
                node->type = JACON_VALUE_STRING;
                node->value.string_val = strdup(current_token.string_val);
                if (node->value.string_val == NULL) return JACON_ERR_MEMORY_ALLOCATION;

                ret = Jacon_consume_token(&current_token, tokenizer, current_index);
                if (ret != JACON_OK) return ret;
            }
            else {
                node->type = JACON_VALUE_STRING;
                node->name = strdup(current_token.string_val);
                if (node->name == NULL) return JACON_ERR_MEMORY_ALLOCATION;

                ret = Jacon_consume_token(&current_token, tokenizer, current_index);
                if (ret != JACON_OK) return ret;

                ret = Jacon_parse_node(node, tokenizer, current_index);
                if (ret != JACON_OK) return ret;
            }
            break;

        case JACON_TOKEN_BOOLEAN:
            node->type = JACON_VALUE_BOOLEAN;
            node->value.bool_val = current_token.bool_val;
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            break;

        case JACON_TOKEN_INT:
            node->type = JACON_VALUE_INT;
            node->value.int_val = current_token.int_val;
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            break;

        case JACON_TOKEN_FLOAT:
            node->type = JACON_VALUE_FLOAT;
            node->value.float_val = current_token.float_val;
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            break;

        case JACON_TOKEN_DOUBLE:
            node->type = JACON_VALUE_DOUBLE;
            node->value.double_val = current_token.double_val;
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            break;

        case JACON_TOKEN_NULL:
            node->type = JACON_VALUE_NULL;
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            break;
            
        case JACON_TOKEN_COLON:
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            return Jacon_parse_node(node, tokenizer, current_index);
        case JACON_TOKEN_COMMA:
            ret = Jacon_consume_token(&current_token, tokenizer, current_index);
            if (ret != JACON_OK) return ret;
            return Jacon_parse_node(node, tokenizer, current_index);
        case JACON_TOKEN_OBJECT_END:
        case JACON_TOKEN_ARRAY_END:
            // Both of these cases should never happen
            // We skip this token when we are done parsing an array / object
        default:
            return JACON_ERR_UNREACHABLE_STATEMENT;
    }
    return JACON_OK;
}

Jacon_Error
Jacon_parse_value(Jacon_Node* root, Jacon_Token token)
{
    switch (token.type) {
        case JACON_TOKEN_STRING:
            root->type = JACON_VALUE_STRING;
            root->value.string_val = strdup(token.string_val);
            if (root->value.string_val == NULL) return JACON_ERR_MEMORY_ALLOCATION;
            break;
        case JACON_TOKEN_INT:
            root->type = JACON_VALUE_INT;
            root->value.int_val = token.int_val;
            break;
        case JACON_TOKEN_DOUBLE:
            root->type = JACON_VALUE_DOUBLE;
            root->value.double_val = token.double_val;
            break;
        case JACON_TOKEN_FLOAT:
            root->type = JACON_VALUE_FLOAT;
            root->value.float_val = token.float_val;
            break;
        case JACON_TOKEN_BOOLEAN:
            root->type = JACON_VALUE_BOOLEAN;
            root->value.bool_val = token.bool_val;
            break;
        case JACON_TOKEN_NULL:
            root->type = JACON_VALUE_NULL;
            break;
        case JACON_TOKEN_ARRAY_START:
        case JACON_TOKEN_ARRAY_END:
        case JACON_TOKEN_OBJECT_START:
        case JACON_TOKEN_OBJECT_END:
        case JACON_TOKEN_COLON:
        case JACON_TOKEN_COMMA:
        default:
            return JACON_ERR_UNREACHABLE_STATEMENT;
    }
    return JACON_OK;
}

/**
 * Parse a Jacon tokenizer content into a queryable C variable
 */
Jacon_Error
Jacon_parse_tokens(Jacon_Node* root, Jacon_Tokenizer* tokenizer)
{
    int ret = JACON_OK;
    if (tokenizer->count == 1) 
        return Jacon_parse_value(root, tokenizer->tokens[0]);
    size_t current_index = 0;
    while(current_index < tokenizer->count) {
        ret = Jacon_parse_node(root, tokenizer, &current_index);
        if (ret != JACON_OK && ret != JACON_NO_MORE_TOKENS) return ret;
    }
    return JACON_OK;
}

Jacon_Error
Jacon_add_node_to_map(Jacon_HashMap* map, Jacon_Node* node, const char* path_to_node)
{
    if (map == NULL || node == NULL)
        return JACON_ERR_NULL_PARAM;

    if (node->type == JACON_VALUE_OBJECT && node->child_count == 0) {
        // No need to add to map since there are no values in the object
        return JACON_OK;
    }

    int ret;

    Jacon_StringBuilder builder = {0};
    ret = Jacon_str_append_null(&builder, path_to_node);
    if (ret != JACON_OK) {
        Jacon_str_free(&builder);
        return ret;
    }
    ret = Jacon_str_append_null(&builder, node->name);
    if (ret != JACON_OK) {
        Jacon_str_free(&builder);
        return ret;
    }
    // If node type is object we do not add yet
    // It will be added when we arrive at the final value
    // We will not allow retrieving a full object
    // Only retriving a single value is allowed for now
    // Single value won't change
    // Full object is subject to change if it appears to be needed
    if (node->type != JACON_VALUE_OBJECT) {
        Jacon_Node* duped = Jacon_duplicate_node(node);
        Jacon_hm_put(map, builder.string, duped);
        Jacon_str_free(&builder);
        return JACON_OK;
    }

    // Add a dot between current path and next name
    // Next name will be appended in next recursive fn call and so on
    if (node->name != NULL) {
        ret = Jacon_str_append_null(&builder, ".");
        if (ret != JACON_OK) {
            Jacon_str_free(&builder);
            return ret;
        }
    }

    for (size_t index = 0; index < node->child_count; index++)
    {
        ret = Jacon_add_node_to_map(map, node->childs[index], builder.string);
        if (ret != JACON_OK) {
            Jacon_str_free(&builder);
            return ret;
        }
    }
    Jacon_str_free(&builder);
    return JACON_OK;    
}

Jacon_Error
Jacon_build_content(Jacon_content* content)
{
    return Jacon_add_node_to_map(&content->entries, content->root, NULL);
}

Jacon_Error
Jacon_free_content(Jacon_content* content)
{
    if (content == NULL)
        return JACON_ERR_NULL_PARAM;

    if (content->root != NULL) {
        Jacon_free_node(content->root);
    }
    Jacon_hm_free(&content->entries);
    return JACON_OK;
}

Jacon_Error
Jacon_get_value_by_name(Jacon_content* content, const char* name, Jacon_ValueType type, void* value)
{
    if (content == NULL || name == NULL || (value == NULL && type != JACON_VALUE_STRING)) {
        return JACON_ERR_NULL_PARAM;
    }
    Jacon_Node* ptr = (Jacon_Node*)Jacon_hm_get(&content->entries, name);
    if (ptr == NULL) {
        return JACON_ERR_KEY_NOT_FOUND;
    }
    switch (type) {
        case JACON_VALUE_STRING:
            *(char**)value = strdup(ptr->value.string_val);
            if (*(char**)value == NULL) return JACON_ERR_MEMORY_ALLOCATION;
            break;
        case JACON_VALUE_INT:
            *(int*)value = ptr->value.int_val;
            break;
        case JACON_VALUE_FLOAT:
            *(float*)value = ptr->value.float_val;
            break;
        case JACON_VALUE_DOUBLE:
            *(double*)value = ptr->value.double_val;
            break;
        case JACON_VALUE_BOOLEAN:
            *(bool*)value = ptr->value.bool_val;
            break;
        case JACON_VALUE_NULL:
        case JACON_VALUE_ARRAY:
        case JACON_VALUE_OBJECT:
        default:
            return JACON_ERR_INVALID_VALUE_TYPE;
    }
    return JACON_OK;
}

Jacon_Error
Jacon_get_string_by_name(Jacon_content* content, const char* name, char** value)
{
    return Jacon_get_value_by_name(content, name, JACON_VALUE_STRING, (void*)value);
}

Jacon_Error
Jacon_get_int_by_name(Jacon_content* content, const char* name, int* value)
{
    return Jacon_get_value_by_name(content, name, JACON_VALUE_INT, value);
}

Jacon_Error
Jacon_get_float_by_name(Jacon_content* content, const char* name, float* value)
{
    return Jacon_get_value_by_name(content, name, JACON_VALUE_FLOAT, value);
}

Jacon_Error
Jacon_get_double_by_name(Jacon_content* content, const char* name, double* value)
{
    return Jacon_get_value_by_name(content, name, JACON_VALUE_DOUBLE, value);
}

Jacon_Error
Jacon_get_bool_by_name(Jacon_content* content, const char* name, bool* value)
{
    return Jacon_get_value_by_name(content, name, JACON_VALUE_BOOLEAN, value);
}

/**
 * Get single value by type
 * 
 * Single value means that there is only this value in json content
 */
Jacon_Error
Jacon_get_value(Jacon_content* content, Jacon_ValueType type, void* value)
{
    if (content == NULL || (value == NULL && type != JACON_VALUE_STRING)) 
        return JACON_ERR_NULL_PARAM;
    switch (type) {
        case JACON_VALUE_STRING:
            *(char**)value = strdup(content->root->value.string_val);
            if (value == NULL) return JACON_ERR_MEMORY_ALLOCATION;
            break;
        case JACON_VALUE_INT:
            *(int*)value = content->root->value.int_val;
            break;
        case JACON_VALUE_FLOAT:
            *(float*)value = content->root->value.float_val;
            break;
        case JACON_VALUE_DOUBLE:
            *(double*)value = content->root->value.double_val;
            break;
        case JACON_VALUE_BOOLEAN:
            *(bool*)value = content->root->value.bool_val;
            break;
        case JACON_VALUE_NULL:
        case JACON_VALUE_ARRAY:
        case JACON_VALUE_OBJECT:
        default:
            return JACON_ERR_INVALID_VALUE_TYPE;
    }
    return JACON_OK;
}

/**
 * Get single string value
 */
Jacon_Error
Jacon_get_string(Jacon_content* content, char** value)
{
    return Jacon_get_value(content, JACON_VALUE_STRING, value);
}

/**
 * Get single int value
 */
Jacon_Error
Jacon_get_int(Jacon_content* content, int* value)
{
    return Jacon_get_value(content, JACON_VALUE_INT, value);
}

/**
 * Get single float value
 */
Jacon_Error
Jacon_get_float(Jacon_content* content, float* value)
{
    return Jacon_get_value(content, JACON_VALUE_FLOAT, value);
}

/**
 * Get single double value
 */
Jacon_Error
Jacon_get_double(Jacon_content* content, double* value)
{
    return Jacon_get_value(content, JACON_VALUE_DOUBLE, value);
}

/**
 * Get single boolean value
 */
Jacon_Error
Jacon_get_bool(Jacon_content* content, bool* value)
{
    return Jacon_get_value(content, JACON_VALUE_BOOLEAN, value);
}

bool
Jacon_exist_by_name(Jacon_content* content, const char* name, Jacon_ValueType type)
{
    void* value = Jacon_hm_get(&content->entries, name);
    return ((Jacon_Node*)value)->type == type;
}

/**
 * Verify the existence of a single string value
 */
Jacon_Error
Jacon_exist_string_by_name(Jacon_content* content, const char* name)
{
    return Jacon_exist_by_name(content, name, JACON_VALUE_STRING);
}

/**
 * Verify the existence of a single int value
 */
Jacon_Error
Jacon_exist_int_by_name(Jacon_content* content, const char* name)
{
    return Jacon_exist_by_name(content, name, JACON_VALUE_INT);
}

/**
 * Verify the existence of a single float value
 */
Jacon_Error
Jacon_exist_float_by_name(Jacon_content* content, const char* name)
{
    return Jacon_exist_by_name(content, name, JACON_VALUE_FLOAT);
}

/**
 * Verify the existence of a single double value
 */
Jacon_Error
Jacon_exist_double_by_name(Jacon_content* content, const char* name)
{
    return Jacon_exist_by_name(content, name, JACON_VALUE_DOUBLE);
}

/**
 * Verify the existence of a single boolean value
 */
Jacon_Error
Jacon_exist_bool_by_name(Jacon_content* content, const char* name)
{
    return Jacon_exist_by_name(content, name, JACON_VALUE_BOOLEAN);
}

/**
 * Verify the existence of a single null value
 */
Jacon_Error
Jacon_exist_null_by_name(Jacon_content* content, const char* name)
{
    return Jacon_exist_by_name(content, name, JACON_VALUE_NULL);
}

/**
 * Verify the existence of a single value of given type
 */
bool
Jacon_exist(Jacon_content* content, Jacon_ValueType type)
{
    return content->root->type == type;
}

/**
 * Verify the existence of a single string value
 */
Jacon_Error
Jacon_exist_string(Jacon_content* content)
{
    return Jacon_exist(content, JACON_VALUE_STRING);
}

/**
 * Verify the existence of a single int value
 */
Jacon_Error
Jacon_exist_int(Jacon_content* content)
{
    return Jacon_exist(content, JACON_VALUE_INT);
}

/**
 * Verify the existence of a single float value
 */
Jacon_Error
Jacon_exist_float(Jacon_content* content)
{
    return Jacon_exist(content, JACON_VALUE_FLOAT);
}

/**
 * Verify the existence of a single double value
 */
Jacon_Error
Jacon_exist_double(Jacon_content* content)
{
    return Jacon_exist(content, JACON_VALUE_DOUBLE);
}

/**
 * Verify the existence of a single boolean value
 */
Jacon_Error
Jacon_exist_bool(Jacon_content* content)
{
    return Jacon_exist(content, JACON_VALUE_BOOLEAN);
}

/**
 * Verify the existence of a single null value
 */
Jacon_Error
Jacon_exist_null(Jacon_content* content)
{
    return Jacon_exist(content, JACON_VALUE_NULL);
}

Jacon_Error
Jacon_append_offset(Jacon_StringBuilder* builder, size_t offset)
{
    for (size_t i = 0; i < offset; i++)
    {
        Jacon_str_append_null(builder, "  ");
    }
    return JACON_OK;
}

/**
 * Get the string representation of a node
 */
Jacon_Error
Jacon_node_as_str(Jacon_Node* node, Jacon_StringBuilder* builder, size_t offset, bool putoffset)
{
    int ret;
    size_t index;

    if (putoffset)
        Jacon_append_offset(builder, offset);

    if (node->name != NULL) {
        Jacon_str_append_null(builder, "\"", node->name, "\": ");
    }
    
    switch (node->type) {
        case JACON_VALUE_OBJECT:
            Jacon_str_append_null(builder, "{");
            if (node->child_count > 0) Jacon_str_append_null(builder, "\n");
            index = 0;
            while (index < node->child_count) {
                ret = Jacon_node_as_str(node->childs[index], builder, offset + 1, true);
                if (ret != JACON_OK) return ret;
                if (++index < node->child_count) Jacon_str_append_null(builder, ",\n");
            }

            if (node->child_count > 0) {
                Jacon_str_append_null(builder, "\n");
                Jacon_append_offset(builder, offset);
            }
            Jacon_str_append_null(builder, "}");
            break;
        case JACON_VALUE_ARRAY:
            Jacon_str_append_null(builder, "[");
            if (node->child_count > 0) {
                Jacon_str_append_null(builder, "\n");
                Jacon_append_offset(builder, offset + 1);
            }

            index = 0;
            while (index < node->child_count) {
                ret = Jacon_node_as_str(node->childs[index], builder, offset + 1, false);
                if (ret != JACON_OK) return ret;
                if (++index < node->child_count) Jacon_str_append_null(builder, ", ");
            }

            if (node->child_count > 0) {
                Jacon_str_append_null(builder, "\n");
                Jacon_append_offset(builder, offset);
            }
            Jacon_str_append_null(builder, "]");
            break;
        case JACON_VALUE_STRING:
            if (node->value.string_val == NULL) return JACON_ERR_NULL_PARAM;
            Jacon_str_append_fmt_null(builder, "\"%s\"", node->value.string_val);
            break;
        case JACON_VALUE_INT:
            Jacon_str_append_fmt_null(builder, "%d", node->value.int_val);
            break;
        case JACON_VALUE_FLOAT:
            Jacon_str_append_fmt_null(builder, "%f", node->value.float_val);
            break;
        case JACON_VALUE_DOUBLE:
            Jacon_str_append_fmt_null(builder, "%f", node->value.double_val);
            break;
        case JACON_VALUE_BOOLEAN:
            Jacon_str_append_fmt_null(builder, "%s", 
                node->value.bool_val ? "true" : "false");
            break;
        case JACON_VALUE_NULL:
            Jacon_str_append_fmt_null(builder, "%s", "null");
            break;
        default:
            break;
    }
    return JACON_OK;
}

Jacon_Error
Jacon_node_as_str_unformatted(Jacon_Node* node, Jacon_StringBuilder* builder)
{
    int ret;
    size_t index;

    if (node->name != NULL) {
        Jacon_str_append_null(builder, "\"", node->name, "\":");
    }
    
    switch (node->type) {
        case JACON_VALUE_OBJECT:
            Jacon_str_append_null(builder, "{");
            index = 0;
            while (index < node->child_count) {
                ret = Jacon_node_as_str_unformatted(node->childs[index], builder);
                if (ret != JACON_OK) return ret;
                if (++index < node->child_count) Jacon_str_append_null(builder, ",");
            }
            Jacon_str_append_null(builder, "}");
            break;
        case JACON_VALUE_ARRAY:
            Jacon_str_append_null(builder, "[");
            index = 0;
            while (index < node->child_count) {
                ret = Jacon_node_as_str_unformatted(node->childs[index], builder);
                if (ret != JACON_OK) return ret;
                if (++index < node->child_count) Jacon_str_append_null(builder, ",");
            }
            Jacon_str_append_null(builder, "]");
            break;
        case JACON_VALUE_STRING:
            if (node->value.string_val == NULL) return JACON_ERR_NULL_PARAM;
            Jacon_str_append_fmt_null(builder, "\"%s\"", node->value.string_val);
            break;
        case JACON_VALUE_INT:
            Jacon_str_append_fmt_null(builder, "%d", node->value.int_val);
            break;
        case JACON_VALUE_FLOAT:
            Jacon_str_append_fmt_null(builder, "%f", node->value.float_val);
            break;
        case JACON_VALUE_DOUBLE:
            Jacon_str_append_fmt_null(builder, "%f", node->value.double_val);
            break;
        case JACON_VALUE_BOOLEAN:
            Jacon_str_append_fmt_null(builder, "%s", 
                node->value.bool_val ? "true" : "false");
            break;
        case JACON_VALUE_NULL:
            Jacon_str_append_fmt_null(builder, "%s", "null");
            break;
        default:
            break;
    }
    return JACON_OK;
}

char *
Jacon_serialize(Jacon_Node* node)
{
    if (node == NULL) return NULL;
    char *str = NULL;
    Jacon_StringBuilder builder = {0};
    if (Jacon_node_as_str(node, &builder, 0, true)) {
        goto ret;
    }
    str = strdup(builder.string);
ret:
    Jacon_str_free(&builder);
    return str;
}

char *
Jacon_serialize_unformatted(Jacon_Node* node)
{
    if (node == NULL) return NULL;
    char *str = NULL;
    Jacon_StringBuilder builder = {0};
    if (Jacon_node_as_str_unformatted(node, &builder)) {
        goto ret;
    }
    str = strdup(builder.string);
ret:    
    Jacon_str_free(&builder);
    return str;
}

Jacon_Error
Jacon_deserialize(Jacon_content* content, const char* str)
{
    if (content == NULL || str == NULL) return JACON_ERR_NULL_PARAM;
    size_t len = strlen(str);
    if (len == 0) return JACON_ERR_EMPTY_INPUT;

    Jacon_Tokenizer tokenizer;
    Jacon_tokenizer_init(&tokenizer);
    Jacon_Error ret;
    ret = Jacon_tokenize(&tokenizer, str);
    if (ret != JACON_OK) {
        Jacon_free_tokenizer(&tokenizer);
        return ret;
    }
    
    // Invalidate empty input
    if (tokenizer.count == 0) {
        Jacon_free_tokenizer(&tokenizer);
        return JACON_ERR_EMPTY_INPUT;
    }

    ret = Jacon_validate_input(&tokenizer);
    if (ret != JACON_OK) {
        Jacon_free_tokenizer(&tokenizer);
        return ret;
    }

    ret = Jacon_parse_tokens(content->root, &tokenizer);
    if (ret != JACON_OK) {
        Jacon_free_tokenizer(&tokenizer);
        return ret;
    }
    Jacon_free_tokenizer(&tokenizer);

    ret = Jacon_build_content(content);
    if (ret != JACON_OK) return ret;

    return JACON_OK;
}