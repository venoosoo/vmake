#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_DS_IMPLEMENTATION
#include "../include/libs/stb_ds.h"

#include "../include/parser.h"
#include "../include/dbgtools.h"
#include "../include/scanner.h"
#include "../include/cache.h"
#include "../include/vmake.h"
#include "../include/compile.h"
  
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

#include <fcntl.h>

void print_help() {
    printf("Usage: vmake [options]\n\n");
    printf("Options:\n");
    printf("  -t, --target <NAME>  Build specifed target (by default builds all targets in order)\n");
    printf("  -j, --jobs <N>       Run <N> parallel compile jobs\n");
    printf("  -f, --file <FILE>    Use specific config file(by defualt vmake.config)\n");
    printf("  -c, --clean          Remove the .vmake cache and build output\n");
    printf("  -B, --always-make    Unconditionally build all targets\n");
    printf("  -q, --quiet          Do not print to stdout\n");
    printf("  -v, --verbose        Prints all debug info\n");
    printf("  -V, --version        Prints current version\n");
    printf("  -h, --help           Print this help message\n");
}


int main(int argc, char** argv) {


    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads < 1) num_threads = 4;
    char *config_file = "vmake.config";
    int verbose = 0;
    struct { char* key; int value; }* targets_name = NULL;

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];


        if (strcmp(arg, "-t") == 0 || strcmp(arg, "--target") == 0) {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
                shput(targets_name, argv[i], 0);
            }
        }
 
        else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_help();
            return 0;
        }
        else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--file") == 0) {
            config_file = argv[++i];
        }
        else if (strcmp(arg, "-c") == 0 || strcmp(arg, "--clean") == 0) {
            clean_dirs();
            return 0;
        }
        else if (strcmp(arg, "-j") == 0 || strcmp(arg, "--jobs") == 0) {
            int user_num = atoi(argv[++i]);
            if (user_num < 1) num_threads = 1;
            num_threads = user_num;
        }
        else if (strcmp(arg, "-B") == 0 || strcmp(arg, "--always-make") == 0) {
            clean_dirs();
        }
        else if (strcmp(arg, "-q") == 0 || strcmp(arg, "--quiet") == 0) {
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull != -1) {
                dup2(devnull, STDOUT_FILENO);
                close(devnull);
            }
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            printf("vmake 1.0.0\n");
            return 0;
        }
        else {
            printf("unkown arg: %s\n",arg);
            exit(1);
        }


    }


    struct FileCache** cache = NULL;

    struct Target** targets = parse(config_file);
    for (size_t i = 0; i < arrlen(targets); i++) {
        struct Target* target = targets[i];
        check_source_dirs(target);
        struct Cache* saved_cache = get_saved_cache(targets[i]->name); //hashmap
        struct IncludeMap* dependency_map = scan(target,saved_cache);
        struct Cache** current_cache = get_current_cache(target,dependency_map,saved_cache); //array
        struct FileCache* file_cache = xmalloc(sizeof(struct FileCache));
        
        
        file_cache->cache = current_cache;
        file_cache->filename = target->name;
        file_cache->dependency_map = dependency_map;
        arrput(cache,file_cache);
        if (shlen(targets_name) < 1 || shgeti(targets_name,targets[i]->name) >= 0) {
            
            char** recompile = compare_cache(current_cache, saved_cache);
            
            if (verbose) {
                print_target(target);
                print_saved_cache(saved_cache);
                print_current_cache(current_cache);
                print_include_map(dependency_map);
                print_recompile(recompile);
            }
            
            struct JobList jobs = {
                .items = recompile,
                .target = target,
                .count = arrlen(recompile),
            };
            ensure_dirs();
            
            if (num_threads > jobs.count) {
                num_threads = jobs.count;
            }
            
            pthread_t* threads = xmalloc(sizeof(pthread_t) * num_threads);
            atomic_store(&job_index, 0);
            
            for (int i = 0; i < num_threads; i++) {
                pthread_create(&threads[i], NULL, worker, &jobs);
            }
            
            for (int i = 0; i < num_threads; i++) {
                pthread_join(threads[i], NULL);
            }
            
            if (jobs.count > 0) {
                link_objects(target);
            } else {
                printf("nothing to do\n");
            }
        }
       
    }
    save_cache(cache);
    return 0;    
}
