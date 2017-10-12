//
// Created by dcat on 10/11/17.
//

#ifndef W2_UEXEC_H
#define W2_UEXEC_H

int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[],
            char *const envp[]);
int execl(const char *path, const char *arg, ...);

int execlp(const char *file, const char *arg, ...);

int execle(const char *path, const char *arg, ...);
#endif //W2_UEXEC_H
