//
// Created by dcat on 10/9/17.
//
#include <print.h>
#include "env.h"

#ifndef KERNEL

extern int main(int argc, char **argv);

int _start(int argc, char **argv, char **envp, uint32_t env_reserved) {
    initenv(envp, env_reserved);
    printf("envp:%x rs:%d\n", envp, env_reserved);
    for (int x = 0;; x++) {
        if (envp[x] == NULL)break;
        printf("%x %x\n", envp[x], &envp[x]);
        printf("env[%d]:%s\n", x, envp[x]);
    }
    main(argc, argv);
}

#endif