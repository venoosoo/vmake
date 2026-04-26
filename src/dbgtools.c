#include "../include/vmake.h"
#include "../include/scanner.h"
#include "../include/libs/stb_ds.h"
#include "../include/cache.h"
#include <stdio.h>
#include <stdlib.h>




void print_target(struct Target* target) {
    printf("TARGET DATA\n\n");
    printf("target_name: %s\n",target->name);
    printf("sources: ");
    for (int i = 0; i < arrlen(target->sources); i++) {
        printf("[%s]",target->sources[i]);
    }
    printf("\n");
    printf("includes: ");
    for (int i = 0; i < arrlen(target->includes); i++) {
        printf("[%s]",target->includes[i]);
    }
    printf("\n");
    printf("output_path: %s\n",target->output_path);
    printf("targeted compiler: %s\n",target->cc);
    printf("compiler flags: ");
    for (int i = 0; i < arrlen(target->flags); i++) {
        printf("[%s]",target->flags[i]);
    }
    printf("\n\n");

}

void print_include_map(struct IncludeMap* map) {
    printf("INCLUDE DATA\n\n");
    for (int i = 0; i < shlen(map); i++) {
        printf("key: {%s}\n", map[i].key);

        printf("values: ");
        for (int j = 0; j < arrlen(map[i].value); j++) {
            printf("[%s]", map[i].value[j]);
        }

        printf("\n");
    }
    printf("\n");
}


void print_current_cache(struct Cache** recompile) {
    if (arrlen(recompile) < 1) {
        printf("NO CURRENT CACHE FOUND\n");
    } else {
        printf("CURRENT CACHE\n");
        for (int i = 0; i < arrlen(recompile); i++) {
            printf("file: %s, hash: %lu\n",recompile[i]->key,recompile[i]->value->hash);
        }
    }
    
}

void print_cachce_data(struct CacheData* data) {
    printf("hash: %lu\n",data->hash);
    printf("dependecies: ");
    for (int i = 0; i < arrlen(data->dependencies); i++) {
        printf("[%s] ",data->dependencies[i]);
    }
    printf("\n");
}

void print_saved_cache(struct Cache* recompile) {
    if (shlen(recompile) < 1) {
        printf("NO SAVED CACHE FOUND\n");
    } else {
        printf("FOUND SAVED CACHE\n");
        for (int i = 0; i < shlen(recompile); i++) {
            printf("file: %s\n",recompile[i].key);
            print_cachce_data(recompile[i].value);
        }
    }
    
}


void print_recompile(char** recompile) {
    if (arrlen(recompile) > 0) {
        printf("FOR RECOMPILE\n");
        for (int i = 0; i < arrlen(recompile); i++) {
            printf("%s\n",recompile[i]);
        }
    } else {
        printf("No new files\n");
    }
}

void free_arr(char** arr) {
    for (int i = 0; i < arrlen(arr); i++) {
        free(arr[i]);
    }
    arrfree(arr);
}

void free_cache_hashmap(struct Cache* saved_cache) {
    for (int i = 0; i < shlen(saved_cache); i++) {
        if (saved_cache[i].value) {
            free_arr(saved_cache[i].value->dependencies);
            free(saved_cache[i].value);
        }

        free(saved_cache[i].key);
    }
    shfree(saved_cache);
}



void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        perror("malloc error");
        exit(EXIT_FAILURE);
    }
    return p;
}