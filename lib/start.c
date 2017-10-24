//
// Created by dcat on 10/9/17.
//
#include <print.h>
#include "env.h"

#ifndef KERNEL
extern int main(int argc, char **argv);

int _start(int argc, char **argv, char **envp, uint32_t env_reserved) {
    for(;;);
    printf("envp:%x rs:%d\n", envp, env_reserved);
    __initenv(envp, env_reserved);
    for (int x = 0;; x++) {
        if (envp[x] == NULL)break;
        printf("env[%d]:%s\n", x, envp[x]);
    }
    int ret=main(argc, argv);
    syscall_exit(ret);
}

#endif