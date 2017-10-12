//
// Created by dcat on 10/11/17.
//

#include <env.h>
#include <str.h>
#include <umem.h>
#include <print.h>
#include <syscall.h>
#include "uexec.h"

extern uchar_t *memcpy(void *dest, void *src, int count);

int find_in_path(const char *file, char *pathbuff) {
    const char *path = getenv("PATH");
    if (path == NULL) {
        printf("warning:PATH is not defined.\n");
        return -1;
    } else {
        int pos = 0, lpos = 0;
        while (pos >= 0) {
            pos = strnxtok(path, ':', lpos);
            if (pos == -1) {
                strcpy(pathbuff, &path[lpos]);
            } else {
                memcpy(pathbuff, &path[lpos], pos - lpos);
                pathbuff[pos - lpos] = 0;
                lpos = pos + 1;
            }
            int tlen = strlen(pathbuff);
            if (pathbuff[tlen - 1] != '/')
                strcat(pathbuff, "/");
            strcats(pathbuff, file, 256);
            int acret = syscall_access(pathbuff, X_OK);
            printf("finding %s %d\n", pathbuff, acret);
            if (acret == 0) {
                return 0;
            }
        }
    }
}

int execve(const char *path, char *const argv[], char *const envp[]) {
    int argc = 0;
    while (argv != NULL && argv[argc] != NULL)argc++;
    syscall_exec(path, argc, argv, envp);
}

int execv(const char *path, char *const argv[]) {
    return execve(path, argv, __getenvp());
}

int execvpe(const char *file, char *const argv[],
            char *const envp[]) {
    char pathbuff[256];
    if (strlen(file) > 0 && file[0] != '/') {
        if (find_in_path(file, pathbuff) != 0)
            strcpy(pathbuff, file);
    } else {
        strcpy(pathbuff, file);
    }
    return execve(pathbuff, argv, envp);
}

int execvp(const char *file, char *const argv[]) {
    return execvpe(file, argv, __getenvp());
}

int execl(const char *path, const char *arg, ...) {
    int argc = 1;
    va_list va;
    va_start(va, arg);
    while (va_arg(va, const char*))argc++;
    char *argv[argc + 1];
    va_start(va, arg);
    for (int x = 1; x < argc; x++) {
        argv[x] = (char *) va_arg(va, const char*);
    }
    argv[0] = (char *) arg;
    argv[argc] = NULL;
    va_arg(va, void*);
    return execv(path, (char *const *) argv);
}

int execlp(const char *file, const char *arg, ...) {
    char pathbuff[256];
    if (find_in_path(file, pathbuff) != 0)
        strcpy(pathbuff, file);
    int argc = 1;
    va_list va;
    va_start(va, arg);
    while (va_arg(va, const char*))argc++;
    char *argv[argc + 1];
    va_start(va, arg);
    for (int x = 1; x < argc; x++) {
        argv[x] = (char *) va_arg(va, const char*);
    }
    argv[0] = (char *) arg;
    argv[argc] = NULL;
    va_arg(va, void*);
    return execv(pathbuff, (char *const *) argv);
}

int execle(const char *path, const char *arg, ...) {
    int argc = 1;
    va_list va;
    va_start(va, arg);
    while (va_arg(va, const char*))argc++;
    char *argv[argc + 1];
    va_start(va, arg);
    for (int x = 1; x < argc; x++) {
        argv[x] = (char *) va_arg(va, const char*);
    }
    argv[0] = (char *) arg;
    argv[argc] = NULL;
    va_arg(va, void*);
    char *const *envp = va_arg(va, char* const*);
    return execve(path, (char *const *) argv, envp);

}