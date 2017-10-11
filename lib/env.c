//
// Created by dcat on 10/7/17.
//


#include <str.h>
#include <print.h>
#include <syscall.h>
#include "env.h"

uint32_t __env_start;
uint32_t __env_rs;

void __initenv(uint32_t start_addr, uint32_t rs) {
    __env_start = start_addr;
    __env_rs = rs;
}

char **__getenvp() {
    return (char **) __env_start;
}

int __getenv(const char *key, char **valueout, int *offsetout) {
    if (__env_start == NULL)return -1;
    int klen = strlen(key);
    char **iter = (char **) __env_start;
    for (int x = 0;; x++) {
        if (iter[x] == 0)break;
        if (strlen(iter[x]) > klen + 1 && strstr(iter[x], key) == 0 && iter[x][klen] == '=') {
            if (valueout)
                *valueout = iter[x];
            if (offsetout)
                *offsetout = x;
            return 0;
        }
    }
    return -1;
}

char *getenv(const char *key) {
    char *value;
    if (__getenv(key, &value, NULL) == 0)
        return &value[strlen(key) + 1];
    else
        return NULL;
}

int setenv(const char *name, const char *value, int overwrite) {
    char **envp = (char **) __env_start;
    char *vspace;
    int vno = -1;
    int len = strlen(name) + strlen(value) + 2;
    if (__getenv(name, &vspace, &vno) == 0) {
        if (!overwrite)return 0;
        if (strlen(value) != strlen(vspace)) {
            syscall_free(vspace);
            vspace = (char *) syscall_malloc(len);
            envp[vno] = vspace;
        }
        sprintf(vspace, "%s=%s", name, value);
        return 0;
    } else {
        vspace = (char *) syscall_malloc(len);
        for (int x = 0; x < __env_rs / sizeof(uint32_t); x++) {
            if (envp[x] == NULL) {
                vno = x;
                break;
            }
        }
        if (vno == -1) {
            //TODO alloc a bigger space
            return -1;
        }
        envp[vno] = vspace;
        sprintf(vspace, "%s=%s", name, value);
        return 0;
    }
}

int unsetenv(const char *name) {
    //TODO
    return -1;
}