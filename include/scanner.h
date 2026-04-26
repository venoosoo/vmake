#pragma once
#include "vmake.h"
#include "cache.h"

struct Cache;

struct IncludeMap {
    char *key; // the .h file
    char **value; // the .c files that using it;
};

struct IncludeMap* scan(struct Target* target, struct Cache* saved_cache);
int check_if_dir(char* path);
