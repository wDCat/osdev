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

int execv(const char *path, char *const argv[]) {
    int argc = 0;
    while (argv != NULL && argv[argc] != NULL)argc++;
    printf("argc:%d\n", argc);
    syscall_exec(path, argc, argv, __getenvp());
}

int execvp(const char *file, char *const argv[]) {
    bool found = false;
    char pathbuff[256];
    if (strlen(file) > 0 && file[0] != '/') {
        const char *path = getenv("PATH");
        if (path == 0) {
            printf("warning:PATH is not defined.\n");
            found = true;
            strcpy(pathbuff, file);
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
                int acret = syscall_access(pathbuff, F_OK);
                printf("finding %s %d\n", pathbuff, acret);
                if (acret == 0) {
                    found = true;
                    break;
                }
            }
        }
    } else {
        found = true;
        strcpy(pathbuff, file);
    }
    if (!found)return -1;
    return execv(pathbuff, argv);
}