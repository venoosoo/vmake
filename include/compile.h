#pragma once
#include <stdatomic.h>

extern atomic_int job_index;

struct JobList {
    char** items;
    struct Target* target;
    int count;
};

void* worker(void* arg);
char* transform_to_object(char* value);
void link_objects(struct Target* target);


void ensure_dirs();
void clean_dirs();