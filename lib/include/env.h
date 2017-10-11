//
// Created by dcat on 10/7/17.
//

#ifndef W2_ENV_H
#define W2_ENV_H

#include <intdef.h>

void __initenv(uint32_t start_addr, uint32_t end_addr);

char **__getenvp();

char *getenv(const char *key);

int setenv(const char *name, const char *value, int overwrite);

int unsetenv(const char *name);

#endif //W2_ENV_H
