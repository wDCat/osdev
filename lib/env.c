//
// Created by dcat on 10/7/17.
//


#include <str.h>
#include <print.h>
#include <screen.h>
#include "env.h"

uint32_t __env_start;
uint32_t __env_rs;

void initenv(uint32_t start_addr, uint32_t rs) {
    __env_start = start_addr;
    __env_rs = rs;
}

char *getenv(const char *key) {
    if (__env_start == NULL) {
        printf("env is not present,,");
        return NULL;
    }
    int klen = strlen(key);
    char **iter = (char **) __env_start;
    for (int x = 0;; x++) {
        if (iter[x] == 0)break;
        if (strlen(iter[x]) > klen + 1 && strstr(iter[x], key) == 0 && iter[x][klen] == '=') {
            return &iter[x][klen+1];
        }
    }
    return NULL;
}
int setenv(const char *name, const char *value, int overwrite){
    TODO;
}
int unsetenv(const char *name){

}