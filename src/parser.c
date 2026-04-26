#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/vmake.h"
#include "../include/libs/stb_ds.h"
#include "../include/scanner.h"
#include "../include/dbgtools.h"

#define WORD_BUFFER_SIZE 400

#define PATH_STRING_SIZE 256

typedef enum TokenType {
    executable,
    includes,
    open_scope,
    close_scope,
    var,
    eq,
    semi,
    open_array,
    close_arary,
    string,
    comma,
} TokenType;


struct Token {
    TokenType type;
    char* value;
};

int is_symbol(int c) {
    return c == '{' || c == '}' ||
           c == '[' || c == ']' ||
           c == '=' || c == ',' ||
           c == ';';
}



struct Token** tokenize(char* config_path) {
    FILE* fptr = fopen(config_path, "r");

    int c;
    char word_buffer[WORD_BUFFER_SIZE];
    memset(word_buffer, 0, WORD_BUFFER_SIZE);
    int word_len = 0;
    struct Token** tokens = NULL; 
    if (fptr) {
        int isString = 0;
        while ((c = fgetc(fptr)) != EOF) {


            if (is_symbol(c)) {
                struct Token* t = xmalloc(sizeof(struct Token));
                switch (c) {
                    case '{': t->type = open_scope; break;
                    case '}': t->type = close_scope; break;
                    case '[': t->type = open_array; break;
                    case ']': t->type = close_arary; break;
                    case '=': t->type = eq; break;
                    case ',': t->type = comma; break;
                    case ';': t->type = semi; break;
                    default:  t->type = var; break;
                }
                word_buffer[word_len] = (char)c;
                char *dup = xmalloc(strlen(word_buffer) + 1);
                strcpy(dup, word_buffer);
                t->value = dup;
                arrput(tokens,t);
                memset(word_buffer, 0, WORD_BUFFER_SIZE);
                word_len = 0;
                continue;
            }

            if (c == 34 && !isString) {
                isString = 1;
                continue;
            }
            if (((c != 32 && c != 10 && c != 34 && c != 59) || (isString && c != 34))) {
                word_buffer[word_len] = (char)c;
                word_len++;
            }
            else {
                if (word_buffer[0] == 0) {
                    continue;
                }
                
                struct Token* t = xmalloc(sizeof(struct Token));
                if (t == NULL) {printf("error allocating token\n"); exit(EXIT_FAILURE);}
                
                
                if (isString && c == 34) {t->type = string; isString = 0;}
                else if (strcmp(word_buffer, "executable") == 0) {t->type = executable;}
                else if (strcmp(word_buffer, "{") == 0) {t->type = open_scope;}
                else if (strcmp(word_buffer, "}") == 0) {t->type = close_scope;}
                else if (strcmp(word_buffer, "=") == 0) {t->type = eq;}
                else if (strcmp(word_buffer, "[") == 0) {t->type = open_array;}
                else if (strcmp(word_buffer, "]") == 0) {t->type = close_arary;}
                else if (strcmp(word_buffer, ",") == 0) {t->type = comma;}
                else if (strcmp(word_buffer, ";") == 0) {t->type = semi;}
                else {t->type = var;}
                char *dup = xmalloc(strlen(word_buffer) + 1);
                strcpy(dup, word_buffer);
                t->value = dup;

                arrput(tokens,t);
                memset(word_buffer, 0, WORD_BUFFER_SIZE);
                word_len = 0;
            }
            
        }


        fclose(fptr);
    }
    else {
        printf("couldnt open file:  %s (by default is: vmake.config)\n",config_path);
        exit(EXIT_FAILURE);
    }
    return tokens;
}

void parse_cc_flags(struct Target* res,struct Token** tokens, char* value) {
    char** flags_arr = NULL;

    if (value != NULL) {
        char* token = strtok(value, " ");
        
        while (token != NULL) {
            char *token_temp = xmalloc(strlen(token) + 1);
            strcpy(token_temp, token);
            arrpush(flags_arr, token_temp);
            
            token = strtok(NULL, " ");
        }
        
    }
    res->flags = flags_arr;
}


void expected(TokenType got,TokenType expected,int* i) {
    if (got != expected) {
        printf("error while parsing at token: %d\n",*i);
        printf("expected: %d, got: %d\n",expected,got);
        exit(1);
    }
    (*i)++;
    return;
}


char** parse_array(struct Token** tokens, int* i) {
    char** arr = NULL;
    (*i)++;
    while (tokens[*i]->type != close_arary) {
        arrput(arr,tokens[*i]->value);
        (*i)++;
        if (tokens[*i]->type == comma) {
            (*i)++;
        }
    }
    (*i)++; // skip ]
    return arr;
}

struct Target* parse_executable_keyword(struct Token** tokens, int *i) {
    struct Target* res = xmalloc(sizeof(struct Target));
    if (strcmp(tokens[*i]->value, "executable") != 0) {
        printf("unkown token while parsing config\n");
        printf("%s\n",tokens[*i]->value);
        exit(EXIT_FAILURE);
    }

    (*i)++; //skip keyword
    char* name = tokens[(*i)++]->value;
    res->name = name;
    expected(tokens[*i]->type, open_scope,i);
    while (tokens[*i]->type != close_scope) {
        char* key = tokens[(*i)++]->value;
        expected(tokens[*i]->type, eq,i);

        if (strcmp(key, "sources") == 0 ) {
            res->sources = parse_array(tokens, i);
        }

        else if (strcmp(key, "cc") == 0) {
            res->cc = tokens[(*i)++]->value;
        }

        else if (strcmp(key, "flags") == 0) {
            parse_cc_flags(res, tokens, tokens[(*i)++]->value);
        }

        else if (strcmp(key, "output") == 0) {
            res->output_path = tokens[*i]->value;
            (*i)++;
        }
        else if (strcmp(key, "includes") == 0) {
            res->includes = parse_array(tokens, i);
        }
        else {
            printf("unkown key: %s",key);
            exit(1);
        }
        expected(tokens[(*i)]->type, semi,i); 
    }
    return res;
}


void check_source_dirs(struct Target* target) {
    for (int i = 0; i < arrlen(target->sources); i++) {
        if (check_if_dir(target->sources[i])) {
            DIR *dir = opendir(target->sources[i]);
            if (!dir) {
                perror("opendir");
                return;
            }
            struct dirent *entry;
            
            while ((entry = readdir(dir)) != NULL) {
                if (is_c_file(entry->d_name)) {
                    char* full_str = xmalloc(PATH_STRING_SIZE);
                    sprintf(full_str, "%s%s",target->sources[i],entry->d_name);
                    arrput(target->sources,full_str);
                }
            }
            closedir(dir);
            arrdel(target->sources,i);
        }
    }
}



struct Target** parse(char* config_path) {


    struct Target** res = NULL;

    struct Token** tokens = tokenize(config_path);

    for (int i = 0; i < arrlen(tokens); i++) {
        struct Target* target = parse_executable_keyword(tokens,&i);
        arrput(res,target);
    }
    return res;

}