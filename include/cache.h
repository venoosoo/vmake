#pragma once

#include <stdint.h>
#include "scanner.h"
#include "vmake.h"
#include "libs/xxhash.h"

struct IncludeMap;

struct CacheData {
    XXH128_hash_t hash;
    int64_t time;
    int64_t size;
    char** dependencies;
};



struct Cache {
    char* key;
    struct CacheData* value;
};

struct FileCache {
    char* filename;
    struct Cache** cache;
    struct IncludeMap* dependency_map;

};


XXH128_hash_t hash_file(char* path);
struct Cache** get_current_cache(struct Target* target, struct IncludeMap* dependency_map, struct Cache* saved_cache); //array
struct Cache* get_saved_cache(char* target_name);
void save_cache(struct FileCache** targets_cache);
char** compare_cache(struct Cache** current_cache, struct Cache* saved_cache);
int is_c_file(const char *filename);
void add_unique(char*** recompile, char* value);