/*
    JSON5 parser module

    License:
    This software is dual-licensed to the public domain and under the following
    license: you are granted a perpetual, irrevocable license to copy, modify,
    publish, and distribute this file as you see fit.
    No warranty is implied, use at your own risk.

    Credits:
    Dominik Madarasz (GitHub: zaklaus)
    r-lyeh (fork)

    Version History:
    2.1.0 - negative numbers fix, comment parsing fix, invalid memaccess fix (@r-lyeh)
    2.0.9 - zpl 4.0.0 support
    2.0.8 - Small cleanup in README and test file
    2.0.7 - Small fixes for tiny cpp warnings
    2.0.5 - Fix for bad access on deallocation
    2.0.4 - Small fix for cpp issues
    2.0.3 - Small bugfix in name with underscores
    2.0.1 - Catch error in name
    2.0.0 - Added basic error handling
    1.4.0 - Added Infinity and NaN constants
    1.3.0 - Added multi-line backtick strings
    1.2.0 - More JSON5 features and bugfixes
    1.1.1 - Small mistake fixed
    1.1.0 - Basic JSON5 support, comments and fixes
    1.0.4 - Header file fixes
    1.0.0 - Initial version
*/

/*

@todo change api to:

json_value * json_parse (const json_char * json, size_t length, char *error); // error = buffer to store error, if any.
void json_value_free(json_value *);

The type field of json_value is one of:

json_array (see ->len, ->values[x])
json_object (see ->len, ->values[x], ->names[x])
json_integer (see ->num)
json_boolean (see ->num)
json_double (see ->dbl)
json_string (see ->len, ->str)
json_null ()

*/

#include "json5.h"
#ifdef __cplusplus
extern "C" {
#endif

// #if JSON5_C
#pragma once

// dual allocator implementations ---------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#if defined(__APPLE__)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

// array based allocator (no enlarge factor)

size_t asize( void *ptr ) {
    return ptr ? ((size_t*)ptr)[-1] : 0;
}

void *arealloc( void *p, size_t sz ) {
    if( !p && sz ) {
        size_t *ret = malloc( sizeof(size_t) + sz );
        *ret = sz;
        return &ret[1];
    }
    if( p && !sz ) {
        free( (size_t*)p - 1 );
        return 0;
    }
    if( p && sz ) {
        size_t slen = asize(p);
        size_t *ret = sz <= slen ? (size_t*)p - 1 : realloc( (size_t*)p - 1, sizeof(size_t) + sz );
        *ret = sz;
        return &ret[1];
    }
    return 0;
}

// vector based allocator (x1.75 enlarge factor)

size_t vcapacity( void *p ) {
    return p ? 1[ (size_t*)p - 2 ] : 0;
}

size_t vsize( void *p ) {
    return p ? 0[ (size_t*)p - 2 ] : 0;
}

void *vrealloc( void *p, size_t sz ) {
    if( !p && sz ) {
        size_t *ret = malloc( sizeof(size_t) * 2 + sz );
        ret[0] = sz;
        ret[1] = 0;
        return &ret[2];
    }
    if( p && !sz ) {
        size_t *ret = (size_t*)p - 2;
        ret[0] = 0;
        ret[1] = 0;
        free( ret );
        return 0;
    }
    if( p && sz ) {
        size_t *ret = (size_t*)p - 2;
        size_t osz = ret[0];
        size_t ocp = ret[1];
        if( sz <= osz ) {
            ret[0] = sz;
            return &ret[2];
        }
        if( sz <= (osz + ocp) ) {
            ret[0] = sz;
            ret[1] = ocp - (sz - osz);
            return &ret[2];
        }
        ret = realloc( ret, sizeof(size_t) * 2 + sz * 1.75 );
        ret[0] = sz;
        ret[1] = (size_t)(sz * 1.75) - sz;
        return &ret[2];
    }
    return 0;
}

// local allocator stuff ------------------------------------------------------

#ifdef _MSC_VER
#define __thread __declspec(thread)
#endif

typedef size_t (*msize_t)(void *);
typedef void* (*realloc_t)(void *, size_t);

// default allocator
// static __thread msize_t   MSIZE   = _msize; // _msize (win), malloc_usable_size (linux), malloc_size (osx)
// static __thread realloc_t REALLOC = realloc;

// portable allocator
// static __thread msize_t   MSIZE   = asize;
// static __thread realloc_t REALLOC = arealloc;

// vector based allocator (enlarge factor x1.75)
static __thread msize_t   MSIZE   = vsize;
static __thread realloc_t REALLOC = vrealloc;

// array library --------------------------------------------------------------

#define array_init(t) ( t = 0 )
#define array_append(t,x) ( t = REALLOC(t, (array_count(t) + 1) * sizeof(t[0]) ), t[ array_count(t) - 1 ] = (x) )
#define array_count(t) ( MSIZE(t) / sizeof(t[0]) )
#define array_free(t) ( REALLOC(t, 0), t = 0 )

// json5 ----------------------------------------------------------------------

inline bool json5__is_special_char(char c) {
    return !!strchr("<>:/", c);
}

inline bool json5__is_control_char(char c) {
    return !!strchr("\"\\/bfnrt", c);
}

inline char *json5__trim(char *str) {
    while (*str && isspace(*str)) {
        ++str;
    }
    return str;
}

inline char *json5__skip(char *str, char c) {
    while ((*str && *str != c) || (*(str - 1) == '\\' && *str == c && json5__is_control_char(c))) {
        ++str;
    }
    return str;
}

inline bool json5__validate_name(char *s, char *err) {
    while (*s) {
        if ((s[0] == '\\' && !json5__is_control_char(s[1])) &&
            (s[0] == '\\' && !isxdigit(s[1]) && !isxdigit(s[2]) && !isxdigit(s[3]) && !isxdigit(s[4]))) {
            *err = *s;
            return false;
        }
        ++s;
    }
    return true;
}

int json5_parse(json5_object *root, char *source, bool strip_comments) {
    assert(root && source);
    char *dest = source;

    if (strip_comments) {
        bool is_lit = false;
        char lit_c = '\0';
        char *p = dest;
        char *b = dest;
        int l = 0;

        while (*p) {
            if (!is_lit) {
                if ((*p == '"' || *p == '\'')) {
                    lit_c = *p;
                    is_lit = true;
                    ++p;
                    continue;
                }
            }
            else {
                /**/ if (*p == '\\' && *(p + 1) && *(p + 1) == lit_c) {
                    p += 2;
                    continue;
                }
                else if (*p == lit_c) {
                    is_lit = false;
                    ++p;
                    continue;
                }
            }

            if (!is_lit) {
                if (p[0] == '/' && p[1] == '*') {
                    b = p;
                    l = 2;
                    p += 2;

                    while (p[0] != '*' && p[1] != '/') {
                        ++p; ++l;
                    }
                    p += 2;
                    l += 2;
                    memset(b, ' ', l);
                }
                if (p[0] == '/' && p[1] == '/' ) { // @rlyeh, was: p[0] == '/' && p[0] == '/'
                    b = p;
                    l = 2;
                    p += 2;

                    while (p[0] != '\n') {
                        ++p; ++l;
                    }
                    ++p;
                    ++l;
                    memset(b, ' ', l);
                }
            }

            ++p;
        }
    }

    int err_code = json5_error_none;
    json5_object root_ = { 0 };
    json5__parse_object(&root_, dest, &err_code);

    *root = root_;
    return err_code;
}

void json5_free(json5_object *obj) {
    /**/ if (obj->type == json5_type_array && obj->elements) {
        for (int i = 0; i < array_count(obj->elements); ++i) {
            json5_free(obj->elements + i);
        }

        array_free(obj->elements);
    }
    else if (obj->type == json5_type_object && obj->nodes) {
        for (int i = 0; i < array_count(obj->nodes); ++i) {
            json5_free(obj->nodes + i);
        }

        array_free(obj->nodes);
    }
}

char *json5__parse_value(json5_object *obj, char *base, int *err_code) {
    assert(obj && base);
    char *p = base;
    char *b = base;
    char *e = base;

    /**/ if (*p == '[') {
        p = json5__trim(p + 1);
        if (*p == ']') return p;
        p = json5__parse_array(obj, p, err_code);

        if (*err_code != json5_error_none) {
            return NULL;
        }

        ++p;
    }
    else if (*p == '{') {
        p = json5__trim(p + 1);
        p = json5__parse_object(obj, p, err_code);

        if (*err_code != json5_error_none) {
            return NULL;
        }

        ++p;
    }
    else if (*p == '"' || *p == '\'') {
        char c = *p;
        obj->type = json5_type_string;
        b = p + 1;
        e = b;
        obj->string = b;

        while (*e) {
            /**/ if (*e == '\\' && *(e + 1) == c) {
                e += 2;
                continue;
            }
            else if (*e == '\\' && (*(e + 1) == '\r' || *(e + 1) == '\n')) {
                *e = ' ';
                e++;
                continue;
            }
            else if (*e == c) {
                break;
            }
            ++e;
        }

        *e = '\0';
        p = e + 1;
    }
    else if (*p == '`') {
        obj->type = json5_type_multistring;
        b = p + 1;
        e = b;
        obj->string = b;

        while (*e) {
            /**/ if (*e == '\\' && *(e + 1) == '`') {
                e += 2;
                continue;
            }
            else if (*e == '`') {
                break;
            }
            ++e;
        }

        *e = '\0';
        p = e + 1;
    }
    else if (isalpha(*p) || (*p == '-' && !isdigit(p[1]))) { // @rlyeh, was: is_digit(*p)

        /**/ if (!strncmp(p, "true", 4)) {
            p += 4;
            obj->type = json5_type_true;
        }
        else if (!strncmp(p, "false", 5)) {
            p += 5;
            obj->type = json5_type_false;
        }
        else if (!strncmp(p, "null", 4)) {
            p += 4;
            obj->type = json5_type_null;
        }
        else if (!strncmp(p, "Infinity", 8)) {
            p += 8;
            obj->type = json5_type_real;
            obj->real = INFINITY;
        }
        else if (!strncmp(p, "-Infinity", 9)) {
            p += 9;
            obj->type = json5_type_real;
            obj->real = -INFINITY;
        }
        else if (!strncmp(p, "NaN", 3)) {
            p += 3;
            obj->type = json5_type_real;
            obj->real = NAN;
        }
        else if (!strncmp(p, "-NaN", 4)) {
            p += 4;
            obj->type = json5_type_real;
            obj->real = -NAN;
        }
        else {
            JSON5_ASSERT; *err_code = json5_error_invalid_value;
            return NULL;
        }
    }
    else if (isdigit(*p) || *p == '+' || *p == '-' || *p == '.') {
        obj->type = json5_type_integer;

        b = p;
        e = b;

        int ib = 0;
        char buf[16] = { 0 };

        /**/ if (*e == '+') ++e;
        else if (*e == '-') {
            buf[ib++] = *e++;
        }

        if (*e == '.') {
            obj->type = json5_type_real;
            buf[ib++] = '0';

            do {
                buf[ib++] = *e;
            } while (isdigit(*++e));
        } else {
            while (isxdigit(*e) || *e == 'x' || *e == 'X') {
                buf[ib++] = *e++;
            }

            if (*e == '.') {
                obj->type = json5_type_real;
                uint32_t step = 0;

                do {
                    buf[ib++] = *e;
                    ++step;
                } while (isdigit(*++e));

                if (step < 2) {
                    buf[ib++] = '0';
                }
            }
        }

        int64_t exp = 0; float eb = 10;
        char expbuf[6] = { 0 };
        int expi = 0;

        if (*e == 'e' || *e == 'E') {
            ++e;
            if (*e == '+' || *e == '-' || isdigit(*e)) {
                if (*e == '-') {
                    eb = 0.1f;
                }

                if (!isdigit(*e)) {
                    ++e;
                }

                while (isdigit(*e)) {
                    expbuf[expi++] = *e++;
                }

            }
            exp = strtol(expbuf, NULL, 10);
        }

        if (*e == '\0') {
            JSON5_ASSERT; *err_code = json5_error_invalid_value;
        }

        // NOTE(ZaKlaus): @enhance
        if (obj->type == json5_type_integer) {
            obj->integer = strtol(buf, 0, 0);

            while (--exp > 0) {
                obj->integer *= (int64_t)eb;
            }
        }
        else {
            obj->real = atof(buf);

            while (--exp > 0) {
                obj->real *= eb;
            }
        }
        p = e;
    }

    return p;
}

char *json5__parse_array(json5_object *obj, char *base, int *err_code) {
    assert(obj && base);
    char *p = base;

    obj->type = json5_type_array;
    array_init(obj->elements);

    while (*p) {
        p = json5__trim(p);

        json5_object elem = { 0 };
        p = json5__parse_value(&elem, p, err_code);

        if (*err_code != json5_error_none) {
            return NULL;
        }

        array_append(obj->elements, elem);

        p = json5__trim(p);

        if (*p == ',') {
            ++p;
            continue;
        }
        else {
            return p;
        }
    }
    return p;
}

char *json5__parse_object(json5_object *obj, char *base, int *err_code) {
    assert(obj && base);
    char *p = base;
    char *b = base;
    char *e = base;

    array_init(obj->nodes);

    p = json5__trim(p);
    if (*p == '{') p++;

    while (*p) {
        json5_object node = { 0 };
        p = json5__trim(p);
        if (*p == '}') return p;

        if (*p == '"' || *p == '\'') {
            if (*p == '"') {
                node.quote_style = json5_style_double_quote;
            }
            else {
                node.quote_style = json5_style_single_quote;
            }

            char c = *p;
            b = ++p;
            e = json5__skip(b, c);
            node.name = b;
            *e = '\0';

            p = ++e;
            p = json5__trim(p);

            if (*p && *p != ':') {
                JSON5_ASSERT; *err_code = json5_error_invalid_name;
                return NULL;
            }
        } else {
            /**/ if (*p == '[') {
                node.name = 0; // @rlyeh, was: *node.name = '\0';
                p = json5__parse_value(&node, p, err_code);
                goto l_parsed;
            }
            else if (isalpha(*p) || *p == '_' || *p == '$') {
                b = p;
                e = b;

                do {
                    ++e;
                } while (*e && (*e == '_' || isalpha(*e) || isdigit(*e))
                    && !isspace(*e) && *e != ':');

                if (*e == ':') {
                    p = e;
                }
                else {
                    while (*e) {
                        if (*e && (!isspace(*e) || *e == ':')) {
                            break;
                        }
                        ++e;
                    }
                    e = json5__trim(e);
                    p = e;

                    if (*p && *p != ':') {
                        JSON5_ASSERT; *err_code = json5_error_invalid_name;
                        return NULL;
                    }
                }

                *e = '\0';
                node.name = b;
                node.quote_style = json5_style_no_quotes;
            }
        }

        char errc;
        if (!json5__validate_name(node.name, &errc)) {
            JSON5_ASSERT; *err_code = json5_error_invalid_name;
            return NULL;
        }

        p = json5__trim(p + 1);
        p = json5__parse_value(&node, p, err_code);

        if (*err_code != json5_error_none) {
            return NULL;
        }

    l_parsed:

        array_append(obj->nodes, node);

        p = json5__trim(p);

        /**/ if (*p == ',') {
            ++p;
            continue;
        }
        else if (*p == '\0' || *p == '}') {
            return p;
        }
        else {
            JSON5_ASSERT; *err_code = json5_error_invalid_value;
            return NULL;
        }
    }
    return p;
}

// #endif // JSON5_C

#ifdef __cplusplus
}
#endif



#ifdef JSON5_BENCH
#include <stdio.h>
#include <omp.h>

char *readfile( const char *file ) {
    char *buf = 0;
    FILE *fp = fopen(file, "rb");
    if( fp ) {
        fseek(fp, 0L, SEEK_END);
        size_t pos = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        buf = (char*)malloc( pos + 1 );
        fread(buf, 1, pos, fp);
        buf[pos] = 0;
        fclose(fp);
    }
    return buf;
}

int main() {
    char *content = readfile("jeopardy.json");
    json5_object root = {0};

    double time = omp_get_wtime();
    json5_error rc = json5_parse(&root, content, false);
    double delta = omp_get_wtime() - time;

    printf("Delta: %fms\n", delta*1000);
    printf("No. of nodes: %td\n", array_count(root.nodes[0].elements));
    printf("Category: %s, air date: %s\nQuestion: %s\n", root.nodes[0].elements[29].nodes[0].string,
           root.nodes[0].elements[29].nodes[1].string,
           root.nodes[0].elements[29].nodes[2].string);

    json5_free(&root);
    free(content);

    for(;0;);
}
#endif


#ifdef JSON5_DEMO

char *source = "/* this is a comment */"
    " nil: null, "
    "\"+ľščťžýáíé=\": true, "
    "\"huge\": 2.2239333e5, "
    "// Hello, new comment \n "
    "\"array\": [+1,2,-3,4,5],     "
    "\"hello\": \"world\", "
    "\"abc\": 42.67, "
    "\"children\" : { \"a\": 1, \"b\": 2 }";

#define ind(x) for (int i = 0; i < x; ++i) printf(" ");

void dump_json_contents(json5_object *o, int indent);

void dump_value(json5_object *o, int indent, bool is_inline, bool is_last) {
    json5_object *node = o;
    indent += 4;

    if (!is_inline) {
        ind(indent);
        printf("\"%s\": ", node->name);
    }

    /**/ if (node->type == json5_type_string) printf("\"%s\"", node->string);
    else if (node->type == json5_type_integer) printf("%lld", node->integer);
    else if (node->type == json5_type_real) printf("%.3llf", node->real);
    else if (node->type == json5_type_object) dump_json_contents(node, indent);
    else if (node->type == json5_type_true) printf("true");
    else if (node->type == json5_type_false) printf("false");
    else if (node->type == json5_type_null) printf("null");
    else /* json5_type_array */ {
        printf("[");
        int elemn = array_count(node->elements);
        for (int j = 0; j < elemn; ++j) {
            dump_value(node->elements + j, -4, true, true);

            if (j < elemn - 1) {
                printf(", ");
            }
        }
        printf("]");
    }

    if (!is_inline) {
        if (!is_last) {
            printf(",\n");
        }
        else {
            printf("\n");
        }
    }
}

void dump_json_contents(json5_object *o, int indent) {
    ind(indent - 4);
    printf("{\n");
    int cnt = array_count(o->nodes);

    for (int i = 0; i < cnt; ++i) {
        if (i < cnt - 1) {
            dump_value(o->nodes + i, indent, false, false);
        }
        else {
            dump_value(o->nodes + i, indent, false, true);
        }
    }

    ind(indent);

    if (indent > 0) {
        printf("}");
    }
    else {
        printf("}\n");
    }
}

#undef ind

int main() {
    json5_object root = { 0 };

    char *d = strdup(source);

    json5_error rc;
    rc = json5_parse(&root, d, true);

    dump_json_contents(&root, 0);

    json5_free(&root);
    free(d);
}

#endif