#pragma once
struct Target {
    char* name;   
    char** sources;
    char* output_path;
    char* cc;
    char** includes;
    char** flags;
    char** linker_flags;
};
 