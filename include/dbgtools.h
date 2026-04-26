#pragma once
#include "vmake.h"
#include "scanner.h"
#include "cache.h"
#include <stddef.h>

void print_target(struct Target* target);
void print_include_map(struct IncludeMap* map);
void print_current_cache(struct Cache** recompile);
void print_saved_cache(struct Cache* recompile);
void print_recompile(char** recompile);
void print_cachce_data(struct CacheData* data);



void *xmalloc(size_t size);

void free_cache_hashmap(struct Cache* saved_cache);