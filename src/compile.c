
#include "../include/libs/stb_ds.h"
#include "../include/compile.h"
#include "../include/vmake.h"
#include "../include/dbgtools.h"
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

atomic_int job_index = 0;

#define STRING_MAX_SIZE 512
#define FLAG_STR_MAX_SIZE 128




char* transform_to_object(char* value) {
    char* slash = strrchr(value, '/');
    char* filename = slash ? slash + 1 : value;

    char* dot = strrchr(filename, '.');
    int name_len = dot ? dot - filename : strlen(filename);

    char result[STRING_MAX_SIZE];
    snprintf(result, sizeof(result), ".vmake/obj/%.*s.o", name_len, filename);

    char* out = xmalloc(strlen(result) + 1);
    strcpy(out, result);
    return out;
}



void* worker(void* arg) {
    struct JobList* jobs = (struct JobList*)arg;
    while (1) {
        int i = atomic_fetch_add(&job_index, 1);


        if (i >= jobs->count) {
            break;
        }
        // compiler + flags + includes + -c + source + -o + output_obj + null
        int arg_size = 1 + arrlen(jobs->target->includes) + arrlen(jobs->target->includes) + 
                       1 + 1 + 1 + 1 + 1;

        char* arg[arg_size];
        int arg_ptr = 0;

        arg[arg_ptr++] = jobs->target->cc;
        for (int i = 0; i < arrlen(jobs->target->flags); i++) {
            arg[arg_ptr] = xmalloc(FLAG_STR_MAX_SIZE);
            snprintf(arg[arg_ptr++], FLAG_STR_MAX_SIZE, "%s", jobs->target->flags[i]);

        }
        for (int i = 0; i < arrlen(jobs->target->includes); i++) {
            arg[arg_ptr] = xmalloc(FLAG_STR_MAX_SIZE);
            snprintf(arg[arg_ptr++], FLAG_STR_MAX_SIZE, "-I%s/", jobs->target->includes[i]);
        }
        arg[arg_ptr++] = "-c";
        arg[arg_ptr++] = jobs->items[i];
        arg[arg_ptr++] = "-o";
        arg[arg_ptr++] = transform_to_object(jobs->items[i]);
        arg[arg_ptr++]= NULL;

        pid_t pid = fork();

        if (pid == 0) {
            execvp(jobs->target->cc, arg);
            perror("execvp failed");
            _exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                printf("Error: failed on %s\n", jobs->items[i]);
            }
        } else {
            perror("fork failed");
        }

    }
    return NULL;
}



void link_objects(struct Target* target) {
    pid_t link_pid = fork();

    if (link_pid == 0) {
        
        // Math for size: CC (1) + Flags (N) + Objects (N) + "-o" (1) + Output Name (1) + NULL (1)
        int arg_size = 1 + arrlen(target->flags) + arrlen(target->sources) + 3;
        
        char** arg = xmalloc(sizeof(char*) * arg_size);
        int arg_ptr = 0;

        arg[arg_ptr++] = target->cc;

        for (int i = 0; i < arrlen(target->flags); i++) {
            arg[arg_ptr++] = target->flags[i];
        }

        for (int i = 0; i < arrlen(target->sources); i++) {
            arg[arg_ptr++] = transform_to_object(target->sources[i]);
        }

        arg[arg_ptr++] = "-o";

        if (target->output_path) {
            char* full_output_path = xmalloc(STRING_MAX_SIZE);
            snprintf(full_output_path, STRING_MAX_SIZE, "%s%s", target->output_path, target->name);
            arg[arg_ptr++] = full_output_path;
        } else {
            arg[arg_ptr++] = target->name;
        }

        arg[arg_ptr++] = NULL;
        mkdir(target->output_path, 0777);


        execvp(arg[0], arg);
        
        perror("Linker execvp failed");
        exit(1);
    } 
    else if (link_pid > 0) {
        int status;
        waitpid(link_pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("[vmake] Build successful!\n");
        } else {
            printf("[vmake] Build failed during linking.\n");
            exit(1);
        }
    } 
    else {
        perror("fork failed during linking");
    }
}


void ensure_dirs() {
    if (mkdir(".vmake", 0755) == -1 && errno != EEXIST) {
        perror("mkdir .vmake failed");
    }

    if (mkdir(".vmake/obj", 0755) == -1 && errno != EEXIST) {
        perror("mkdir .vmake/obj failed");
    }
}

void clean_dirs() {
    int ret;

    // Remove object directory and its contents
    ret = system("rm -rf .vmake/obj");
    if (ret != 0) {
        perror("failed to remove .vmake/obj");
    }

    // Remove main directory
    ret = system("rm -rf .vmake");
    if (ret != 0) {
        perror("failed to remove .vmake");
    }

    // Remove cache file
    if (remove(".vmake_cache") != 0) {
        perror("failed to remove .vmake_cache");
    }
}
