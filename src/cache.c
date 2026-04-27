#include <dirent.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../include/libs/xxhash.h"
#include "../include/libs/stb_ds.h"
#include "../include/vmake.h"
#include "../include/cache.h"
#include "../include/scanner.h"
#include "../include/dbgtools.h"


#define PATH_STRING_SIZE 256


int is_c_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 3) return 0;
    if (len >= 2 && strcmp(filename + len - 2, ".c") == 0) return 1;
    if (len >= 4 && strcmp(filename + len - 4, ".cpp") == 0) return 1;
    if (len >= 3 && strcmp(filename + len - 3, ".cc") == 0) return 1;
    if (len >= 4 && strcmp(filename + len - 4, ".cxx") == 0) return 1;
    return 0;
}


XXH128_hash_t hash_file(char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("fopen failed");
        printf("path = %s\n", path);
        
        // Return a zeroed-out struct on failure
        XXH128_hash_t zero_hash = {0, 0};
        return zero_hash;
    }

    XXH3_state_t* state = XXH3_createState();
    if (state == NULL) {
        fclose(f);
        XXH128_hash_t zero_hash = {0, 0};
        return zero_hash;
    }

    XXH3_128bits_reset(state);

    char buffer[4096];
    size_t n;

    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        XXH3_128bits_update(state, buffer, n);
    }

    fclose(f);

    XXH128_hash_t hash = XXH3_128bits_digest(state);
    XXH3_freeState(state);

    return hash;
}

void bdg_some() {

}


void save_cache(struct FileCache** targets_cache) {
    FILE* cache_file = fopen(".vmake_cache", "w");
    for (size_t i = 0; i < arrlen(targets_cache) ; i++) {
        fprintf(cache_file, "[%s]\n",targets_cache[i]->filename);
        for (size_t j = 0; j < arrlen(targets_cache[i]->cache); j++) {
            char* key = targets_cache[i]->cache[j]->key;
            struct IncludeMap* map = targets_cache[i]->dependency_map;
            XXH128_hash_t hash = targets_cache[i]->cache[j]->value->hash;
            fprintf(cache_file, "%s %016lx%016lx %ld %ld > ", key, hash.high64,hash.low64, targets_cache[i]->cache[j]->value->time, targets_cache[i]->cache[j]->value->size);
            struct IncludeMap* file_data = shgetp_null(map,key);
            if (file_data) {
                for (size_t k = 0; k < arrlen(file_data->value); k++) {
                    fprintf(cache_file, "%s ",file_data->value[k]);
                }
                fprintf(cache_file,"\n");
            } else {
                fprintf(cache_file, "\n");
            }
        }
    }

    fclose(cache_file);
}


struct Cache* get_cache(char* file_path) {
    struct Cache* file_cache = xmalloc(sizeof(struct Cache));
    file_cache->key = file_path;
    struct CacheData* data_ptr = xmalloc(sizeof(struct CacheData));
    data_ptr->hash = hash_file(file_path);
    data_ptr->dependencies = NULL;
    struct stat file_stat;
    stat(file_path, &file_stat);
    data_ptr->time = file_stat.st_mtime;
    data_ptr->size = file_stat.st_size;
    file_cache->value = data_ptr;
    return file_cache;
}


struct Cache** get_current_cache(struct Target* target, struct IncludeMap* dependency_map, struct Cache* saved_cache) {
    struct Cache** cache_arr = NULL;
    for (size_t i = 0; i < arrlen(target->sources); i++) {
        struct stat file_stat;
        stat(target->sources[i],&file_stat);
        if (saved_cache && saved_cache[i].value->time == file_stat.st_mtime && 
            saved_cache[i].value->size ==  file_stat.st_size ) {
                arrput(cache_arr,&saved_cache[i]);
                continue;
        }
        struct Cache* cache = get_cache(target->sources[i]);
        arrput(cache_arr,cache);        
    }
    for (size_t i = 0; i < shlenu(dependency_map); i++) {
        struct Cache* cache = get_cache(dependency_map[i].key);
        arrput(cache_arr,cache);
    }

    return cache_arr;
}


void add_unique(char*** recompile, char* value) {
    for (size_t i = 0; i < arrlen(*recompile); i++) {
        if (strcmp(value, (*recompile)[i]) == 0) {
            return;
        }
    }
    arrput(*recompile,value);
}




struct Cache* get_saved_cache(char* target_name) {
    struct Cache* cache = NULL;
    FILE* fptr = fopen(".vmake_cache", "r");
    if (fptr == NULL) return NULL;

    char buffer[512];
    char section[256];
    snprintf(section, sizeof(section), "[%s]", target_name);

    // find our section
    int found = 0;
    while (fgets(buffer, sizeof(buffer), fptr) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, section) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        fclose(fptr);
        return NULL;
    }

    // parse until next section or EOF
    while (fgets(buffer, sizeof(buffer), fptr) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (buffer[0] == '[') break;

        char* file = xmalloc(256);
        unsigned long long hash_high, hash_low;
        int offset = 0;
        
        if (sscanf(buffer, "%255s %016llx%016llx > %n", file, &hash_high, &hash_low, &offset) >= 3) {
            char** dependencies = NULL;
            
            if (offset > 0 && buffer[offset] != '\0') {
                char* temp = buffer + offset;
                char* token = strtok(temp, " ");
                while (token != NULL) {
                    char* dep = xmalloc(strlen(token) + 1);
                    strcpy(dep, token);
                    arrput(dependencies, dep);
                    token = strtok(NULL, " ");
                }
            }
            
            struct CacheData* data = xmalloc(sizeof(struct CacheData));
            
            data->hash.high64 = hash_high;
            data->hash.low64  = hash_low;
            
            data->dependencies = dependencies;
            
            shput(cache, file, data);
        } else {
            free(file); 
        }
    }

    fclose(fptr);
    return cache;
}


char** compare_cache(struct Cache** current_cache, struct Cache* saved_cache) {
    char** recompile = NULL;
    for (int i = 0; i < arrlen(current_cache); i++) {
        struct Cache* curr_cache = current_cache[i];
        struct Cache* found_cache = shgetp_null(saved_cache,curr_cache->key); 
        if (found_cache) {
                if (!XXH128_isEqual(curr_cache->value->hash, found_cache->value->hash)) {
                    add_unique(&recompile, found_cache->key);
                }
        } else {
            if (is_c_file(curr_cache->key)) {
                add_unique(&recompile, curr_cache->key);
            }
        }
    }
    return recompile;

}