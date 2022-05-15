/**
 * Copyright (c) 2016 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "ini_rw.h"

    struct ini_t {
        char* data;
        char* end;
    };


    /* Case insensitive string compare */
    static int strcmpci(const char* a, const char* b) {
        for (;;) {
            int d = tolower(*a) - tolower(*b);
            if (d != 0 || !*a || !*b) {
                return d;
            }
            a++, b++;
        }
    }

    /* Returns the next string in the split data */
    static char* next(const ini_t* ini, char* p) {
        p += strlen(p);
        while (p < ini->end && *p == '\0') {
            p++;
        }
        return p;
    }

    static void trim_back(ini_t* ini, char* p) {
        while (p >= ini->data && (*p == ' ' || *p == '\t' || *p == '\r')) {
            *p-- = '\0';
        }
    }

    static char* discard_line(ini_t* ini, char* p) {
        while (p < ini->end && *p != '\n') {
            *p++ = '\0';
        }
        return p;
    }


    static char* unescape_quoted_value(ini_t* ini, char* p) {
        /* Use `q` as write-head and `p` as read-head, `p` is always ahead of `q`
         * as escape sequences are always larger than their resultant data */
        char* q = p;
        p++;
        while (p < ini->end && *p != '"' && *p != '\r' && *p != '\n') {
            if (*p == '\\') {
                /* Handle escaped char */
                p++;
                switch (*p) {
                default: *q = *p;    break;
                case 'r': *q = '\r';  break;
                case 'n': *q = '\n';  break;
                case 't': *q = '\t';  break;
                case '\r':
                case '\n':
                case '\0': goto end;
                }

            }
            else {
                /* Handle normal char */
                *q = *p;
            }
            q++, p++;
        }
    end:
        return q;
    }


    /* Splits data in place into strings containing section-headers, keys and
     * values using one or more '\0' as a delimiter. Unescapes quoted values */
    static void split_data(ini_t* ini) {
        char* value_start, * line_start;
        char* p = ini->data;

        while (p < ini->end) {
            switch (*p) {
            case '\r':
            case '\n':
            case '\t':
            case ' ':
                *p = '\0';
                /* Fall through */

            case '\0':
                p++;
                break;

            case '[':
                p += strcspn(p, "]\n");
                *p = '\0';
                break;

            case ';':
                p = discard_line(ini, p);
                break;

            default:
                line_start = p;
                p += strcspn(p, "=\n");

                /* Is line missing a '='? */
                if (*p != '=') {
                    p = discard_line(ini, line_start);
                    break;
                }
                trim_back(ini, p - 1);

                /* Replace '=' and whitespace after it with '\0' */
                do {
                    *p++ = '\0';
                } while (*p == ' ' || *p == '\r' || *p == '\t');

                /* Is a value after '=' missing? */
                if (*p == '\n' || *p == '\0') {
                    p = discard_line(ini, line_start);
                    break;
                }

                if (*p == '"') {
                    /* Handle quoted string value */
                    value_start = p;
                    p = unescape_quoted_value(ini, p);

                    /* Was the string empty? */
                    if (p == value_start) {
                        p = discard_line(ini, line_start);
                        break;
                    }

                    /* Discard the rest of the line after the string value */
                    p = discard_line(ini, p);

                }
                else {
                    /* Handle normal value */
                    p += strcspn(p, "\n");
                    trim_back(ini, p - 1);
                }
                break;
            }
        }
    }


    ini_t* ini_load(const char* filename) {
        ini_t* ini = NULL;
        char* data = NULL;
        FILE* fp = NULL;
        size_t n, sz;

        /* Create empty ini struct when no file is requested */
        if (!filename || !strcmp(filename, "")) {
            ini = ini_create(NULL, 0u);
            return ini;
        }

        /* Open file */
        fp = fopen(filename, "rb");
        if (!fp) {
            goto fail;
        }

        /* Get file size */
        fseek(fp, 0, SEEK_END);
        sz = ftell(fp);
        rewind(fp);

        /* Load file content into memory */
        data = malloc(sz);
        if (!data) {
            goto fail;
        }
        n = fread(data, 1, sz, fp);
        if (n != sz) {
            goto fail;
        }

        /* Create ini struct with data read from file */
        ini = ini_create(data, sz);

        /* Clean up and return */
        free(data);
        fclose(fp);
        return ini;

    fail:
        if (data) free(data);
        if (fp) fclose(fp);
        if (ini) ini_free(ini);
        return NULL;
    }


    ini_t* ini_create(const char* data, const size_t sz) {
        ini_t* ini = NULL;

        /* Create ini struct with passed in data */
        if (data) {
            /* Init ini struct */
            ini = calloc(1u, sizeof(*ini));
            if (!ini) {
                goto fail;
            }

            /* Set pointers and null terminate */
            char* new_data = malloc(sz + 1u);
            if (!new_data) {
                goto fail;
            }
            memcpy(new_data, data, sz);
            new_data[sz] = '\0';

            ini->data = new_data;
            ini->end = new_data + sz;
        }
        /* Create empty ini */
        else if (!data) {
            /* Init ini struct */
            ini = calloc(1u, sizeof(*ini));
            if (!ini) {
                goto fail;
            }

            /* Set pointers and null terminate */
            ini->data = calloc(1u, 1u);
            if (!ini->data) {
                goto fail;
            }
            ini->end = ini->data;
        }
        else {
            goto fail;
        }

        /* Prepare data */
        split_data(ini);

        /* Return */
        return ini;

    fail:
        if (ini) ini_free(ini);
        return NULL;
    }


    int ini_save(const ini_t* ini, const char* filename) {
        FILE* fp = NULL;
        char* p = ini->data;
        int firstsection = 1;

        /* Open file */
        fp = fopen(filename, "wb");
        if (!fp) {
            goto fail;
        }

        /* Save ini struct to a file */
        while (p < ini->end) {
            if (*p == '[') {
                if (firstsection) {
                    firstsection = 0;
                }
                else {
                    fputc('\n', fp);
                }
                fprintf(fp, "%s]\n", p);
            }
            else {
                fprintf(fp, "%s = \"", p);

                p = next(ini, p);
                char* c = p;
                while (c < ini->end && *c != '\0') {
                    if (*c == '"' || *c == '\r' || *c == '\n' || *c == '\t' || *c == '\\') {
                        fprintf(fp, "\\");
                        switch (*c) {
                        case '\r': fprintf(fp, "r");      break;
                        case '\n': fprintf(fp, "n");      break;
                        case '\t': fprintf(fp, "t");      break;
                        default: fprintf(fp, "%c", *c); break;
                        }
                    }
                    else {
                        fprintf(fp, "%c", *c);
                    }
                    c++;
                }
                fprintf(fp, "\"\n");
            }
            p = next(ini, p);
        }
        fclose(fp);
        return 1;

    fail:
        if (fp) fclose(fp);
        return 0;
    }


    char* ini_tostring(const ini_t* ini) {
        if (ini->data == ini->end) {
            return calloc(1u, sizeof(char));
        }

        char* p;
        int firstsection;

        firstsection = 1;
        const size_t nlsz = snprintf(NULL, 0u, "\n");
        p = ini->data;
        size_t sz = 0u;
        while (p < ini->end) {
            if (*p == '[') {
                if (firstsection) {
                    firstsection = 0;
                }
                else {
                    sz++;
                }
                sz += strlen(p) + 1 + nlsz;
            }
            else {
                sz += strlen(p) + 4;
                p = next(ini, p);
                for (char* c = p; c < ini->end && *c != '\0'; c++) {
                    if (*c == '"' || *c == '\r' || *c == '\n' || *c == '\t' || *c == '\\') {
                        sz += 2;
                    }
                    else {
                        sz++;
                    }
                }
                sz += 1 + nlsz;
            }
            p = next(ini, p);
        }
        sz++;

        firstsection = 1;
        p = ini->data;
        char* printout = malloc(sz);
        char* pos = printout;
        while (p < ini->end) {
            if (*p == '[') {
                if (firstsection) {
                    firstsection = 0;
                }
                else {
                    pos += sprintf(pos, "\n");
                }
                pos += sprintf(pos, "%s]", p);
                *pos = '\n';
                pos++;
                *pos = '\0';
            }
            else {
                pos += sprintf(pos, "%s = \"", p);

                p = next(ini, p);
                for (char* c = p; c < ini->end && *c != '\0'; c++) {
                    if (*c == '"' || *c == '\r' || *c == '\n' || *c == '\t' || *c == '\\') {
                        pos += sprintf(pos, "\\");
                        switch (*c) {
                        case '\r': pos += sprintf(pos, "r");      break;
                        case '\n': pos += sprintf(pos, "n");      break;
                        case '\t': pos += sprintf(pos, "t");      break;
                        default: pos += sprintf(pos, "%c", *c); break;
                        }
                    }
                    else {
                        pos += sprintf(pos, "%c", *c);
                    }
                }
                pos += sprintf(pos, "\"\n");
            }
            p = next(ini, p);
        }

        return printout;
    }

    int ini_merge(ini_t* dst, const ini_t* src) {
        char* p = src->data;
        char* section = NULL;

        while (p < src->end) {
            if (*p == '[') {
                section = p + 1;
            }
            else {
                if (section == NULL) {
                    return 0;
                }
                char* val = next(src, p);
                if (!ini_set(dst, section, p, val)) {
                    return 0;
                }
            }
            p = next(src, p);
        }

        return 1;
    }

    ini_t* ini_copy(const ini_t* ini) {
        if (!ini) {
            return NULL;
        }

        ini_t* copy = calloc(1u, sizeof(*ini));
        if (!copy) goto fail;

        copy->data = malloc((ini->end - ini->data) + 1u);
        if (!copy->data) goto fail;

        memcpy(copy->data, ini->data, (ini->end - ini->data) + 1u);
        copy->end = copy->data + (ini->end - ini->data);

        return copy;

    fail:
        if (copy) ini_free(copy);
        return NULL;
    }


    void ini_free(ini_t* ini) {
        if (!ini) return;
        free(ini->data);
        free(ini);
    }


    const char* ini_get(const ini_t* ini, const char* section, const char* key) {
        char* current_section = "";
        char* val;
        char* p = ini->data;

        if (*p == '\0') {
            p = next(ini, p);
        }

        while (p < ini->end) {
            if (*p == '[') {
                /* Handle section */
                current_section = p + 1;

            }
            else {
                /* Handle key */
                val = next(ini, p);
                if (!section || !strcmpci(section, current_section)) {
                    if (!strcmpci(p, key)) {
                        return val;
                    }
                }
                p = val;
            }

            p = next(ini, p);
        }

        return NULL;
    }


    int ini_sget(
        const ini_t* ini, const char* section, const char* key,
        const char* scanfmt, ...
    ) {
        int n = EOF;
        const char* val = ini_get(ini, section, key);
        if (val && scanfmt) {
            va_list args_list;
            va_start(args_list, scanfmt);
            n = vsscanf(val, scanfmt, args_list);
            va_end(args_list);
        }
        return n;
    }

    static int replace(ini_t* ini, char* dst, const char* src, size_t dstsz, size_t srcsz) {
        const size_t dstdatasz = ini->end - ini->data + 1;
        const size_t srcdatasz = dstdatasz - dstsz + srcsz;

        /* Best case, where we can just overwrite the old value with no change in data size */
        if (dstsz == srcsz) {
            memcpy(dst, src, srcsz);
            return 1;
        }
        /* Memmove the data after the old value right after where the new value goes, put in the new value, then realloc down to the new size */
        else if (dstsz > srcsz) {
            memmove(dst + srcsz, dst + dstsz, (size_t)(ini->end - dst) - dstsz + 1);
            memcpy(dst, src, srcsz);
            char* data = realloc(ini->data, srcdatasz);
            if (!data) {
                return 0;
            }
            ini->data = data;
            ini->end = ini->data + srcdatasz - 1;
        }
        /* We realloc to the larger size, memmove up the data after the old value, then put in the new value */
        else {
            const ptrdiff_t dst_offset = dst - ini->data;
            const ptrdiff_t sz = ini->end - ini->data;
            char* data = realloc(ini->data, srcdatasz);
            if (!data) {
                return 0;
            }
            ini->data = data;
            ini->end = ini->data + sz;
            /* It's not guaranteed that realloc returns the same pointer, so this guarantees the dst pointer will work */
            dst = ini->data + dst_offset;
            memmove(dst + srcsz, dst + dstsz, (size_t)(ini->end - dst) - dstsz + 1);
            memcpy(dst, src, srcsz);
            ini->end = ini->data + srcdatasz - 1;
        }

        return 1;
    }

    int ini_set(ini_t* ini, const char* section, const char* key, const char* val) {
        if (!ini || !section || !key || !val) {
            return 0;
        }

        char* current_section = NULL;
        char* p = ini->data;
        const size_t sectionsz = strlen(section) + 1;
        const size_t keysz = strlen(key) + 1;
        const size_t valsz = strlen(val) + 1;
        const size_t keyvalsz = keysz + valsz;

        if (*p == '\0') {
            p = next(ini, p);
        }

        while (p < ini->end) {
            if (*p == '[') {
                /* Handle section, knowing it precedes another section */
                if (current_section) {
                    /* The section for the value exists, but the key doesn't, so add the key to the start of the existing section */
                    char* section_start = next(ini, current_section);
                    char* keyval = malloc(keyvalsz);
                    if (!keyval) {
                        return 0;
                    }
                    strncpy(keyval, key, keysz);
                    strncpy(keyval + keysz, val, valsz);
                    int success = replace(ini, section_start, keyval, 0, keyvalsz);
                    free(keyval);
                    return success;
                }
                if (!strcmpci(section, p + 1)) {
                    current_section = p;
                }
                else {
                    current_section = NULL;
                }
            }
            else {
                char* oldval = next(ini, p);
                if (current_section) {
                    if (!strcmpci(key, p)) {
                        /* Remove the value */
                        if (!val || !strcmpci(val, "")) {
                            char* end = next(ini, oldval);
                            /* Value being removed is the last in the section, so remove the section too */
                            if (next(ini, current_section) == p && (end == ini->end || *end == '[')) {
                                return replace(ini, current_section, current_section, end - current_section, 0);
                            }
                            /* The section contains other values besides the one being removed, so only remove the requested key */
                            else {
                                return replace(ini, p, p, end - p, 0);
                            }
                        }
                        /* Change the existing key's value to the input value */
                        else {
                            return replace(ini, oldval, val, strlen(oldval) + 1, strlen(val) + 1);
                        }
                    }
                    p = next(ini, p);
                }
            }

            p = next(ini, p);
        }

        /* Did not find the key previously, and requested the key be deleted, so just return if the value is empty. */
        if (!val || !strcmpci(val, "")) {
            return 1;
        }

        /* Handle section, knowing it's the last in the struct */
        if (current_section) {
            /* The section for the value exists, but the key doesn't, so add the key to the start of the existing section */
            char* section_start = next(ini, current_section);
            char* keyval = malloc(keyvalsz);
            if (!keyval) {
                return 0;
            }
            strncpy(keyval, key, keysz);
            strncpy(keyval + keysz, val, valsz);
            int success = replace(ini, section_start, keyval, 0, keyvalsz);
            free(keyval);
            return success;
        }
        else {
            /* The section doesn't exist, so create it and add the value */
            const size_t new_sectionsz = 1 + sectionsz + keyvalsz;
            char* new_section = malloc(new_sectionsz);
            if (!new_section) {
                return 0;
            }
            new_section[0] = '[';
            strncpy(new_section + 1, section, sectionsz);
            strncpy(new_section + 1 + sectionsz, key, keysz);
            strncpy(new_section + 1 + sectionsz + keysz, val, valsz);
            int success = replace(ini, p, new_section, 0, new_sectionsz);
            free(new_section);
            return success;
        }
    }

    int ini_pset(ini_t* ini, const char* section, const char* key, const char* printfmt, ...) {
        va_list args_list1;
        va_start(args_list1, printfmt);
        va_list args_list2;
        va_copy(args_list2, args_list1);
        const size_t sz = vsnprintf(NULL, 0, printfmt, args_list1);
        va_end(args_list1);
        char* val = malloc(sz + 1);
        vsnprintf(val, sz + 1, printfmt, args_list2);
        va_end(args_list2);
        int success;
        if (printfmt) {
            success = ini_set(ini, section, key, val);
        }
        else {
            success = 0;
        }
        free(val);
        return success;
    }

#ifdef __cplusplus
}
#endif