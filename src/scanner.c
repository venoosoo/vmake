#include "../include/vmake.h"
#include "../include/scanner.h"
#include "../include/libs/stb_ds.h"
#include "../include/cache.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dbgtools.h"
#include <dirent.h>



char* resolve_path(struct Target* target, char* include_path) {
    char* path = include_path;

    while (path[0] == '.' && path[1] == '.' && path[2] == '/') {
        path += 3;
    }

    char* slash = strchr(path, '/');
    if (slash != NULL) {
        path = slash + 1;
    }

    for (int i = 0; i < arrlen(target->includes); i++) {
        char full[512];
        snprintf(full, sizeof(full), "%s%s", target->includes[i], path);

        FILE* f = fopen(full, "r");
        if (f != NULL) {
            fclose(f);
            char* result = xmalloc(strlen(full) + 1);
            strcpy(result, full);
            return result;
        }
    }
    return include_path;
}


int check_if_dir(char* path) {
    if (path == NULL || *path == '\0') {
        return 0;
    }

    int len = strlen(path);
    return path[len - 1] == '/';
}



void scan_file(struct Target* target,struct IncludeMap** hash_map,char* source) {
    FILE* fptr = fopen(source, "r");
    if (fptr == NULL) {
        printf("couldnt open the source target file: %s\n", source);

    }

    char** arr = NULL;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), fptr) != NULL) {
        if (strstr(buffer, "#include") != NULL) {
            char* ptr = buffer;

            for (int j = 0; j < arrlen(arr); j++) {
                char* resolved_path = resolve_path(target, arr[j]);
                if (resolved_path == arr[j]) continue; // not found, skip
                struct IncludeMap* res = shgetp_null(*hash_map, resolved_path);
                if (res) {
                    arrput(res->value, source);
                } else {
                    char** new_arr = NULL;
                    arrput(new_arr, source);
                    shput(*hash_map, resolved_path, new_arr);
                }
            }

            while (*ptr == ' ' || *ptr == '\t') ptr++;

            ptr += strlen("#include");

            while (*ptr == ' ' || *ptr == '\t') ptr++;

            if (*ptr != '"') continue;
            ptr++;

            char* end = strchr(ptr, '"');
            if (end == NULL) continue;

            int len = end - ptr;
            char* include_path = xmalloc(len + 1);
            strncpy(include_path, ptr, len);
            include_path[len] = '\0';

            arrpush(arr, include_path);
        }
    }


    for (int j = 0; j < arrlen(arr); j++) {
        char* resolved_path = resolve_path(target, arr[j]);
        struct IncludeMap* res = shgetp_null(*hash_map,resolved_path);
        if (res) {
            arrput(res->value,source);
        } else {
            char** new_arr = NULL;
            arrput(new_arr,source);
            shput(*hash_map,resolved_path,new_arr);
        }
    }


    arrfree(arr);
    fclose(fptr);
}


struct Ccache {
    char* key;
    uint64_t value;
};





struct IncludeMap* scan(struct Target* target, struct Cache* saved_cache) {
    struct IncludeMap* hash_map = NULL;
    struct Ccache* c_cache = NULL;
    struct Ccache* to_rescan = NULL;

    if (shlen(saved_cache) < 1) {
        for (int i = 0; i < arrlen(target->sources); i++) {
            scan_file(target, &hash_map, target->sources[i]);
        }
        return hash_map;
    }

    for (int i = 0; i < shlen(saved_cache); i++) {
        struct Cache check = saved_cache[i];
        if (is_c_file(check.key)) {
            if (hash_file(check.key) != check.value->hash) {
                if (shgeti(to_rescan, check.key) < 0)
                    shput(to_rescan, check.key, check.value->hash);
            }
            shput(c_cache, check.key, check.value->hash);
        } else {
            if (hash_file(resolve_path(target, check.key)) != check.value->hash) {
                for (int j = 0; j < arrlen(check.value->dependencies); j++) {
                    char* dep = check.value->dependencies[j];
                    if (shgeti(to_rescan, dep) < 0)
                        shput(to_rescan, dep, 0);
                }
                continue;
            }
            char** depend_arr = NULL;
            for (int j = 0; j < arrlen(check.value->dependencies); j++) {
                struct Ccache* hash = shgetp_null(c_cache, check.value->dependencies[j]);
                if (hash) {
                    if (hash_file(check.value->dependencies[j]) == hash->value) {
                        arrput(depend_arr, check.value->dependencies[j]);
                    } else {
                        char* dep = check.value->dependencies[j];
                        if (shgeti(to_rescan, dep) < 0)
                            shput(to_rescan, dep, 0);
                    }
                }
            }
            shput(hash_map, check.key, depend_arr);
        }
    }

    for (int i = 0; i < shlen(to_rescan); i++) {
        scan_file(target, &hash_map, to_rescan[i].key);
    }

    shfree(to_rescan);
    return hash_map;
}

