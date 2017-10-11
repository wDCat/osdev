//
// Created by dcat on 10/11/17.
//

#ifndef W2_UEXEC_H
#define W2_UEXEC_H

int execvp(const char *file, char *const argv[]);

int execv(const char *path, char *const argv[]);

#endif //W2_UEXEC_H
