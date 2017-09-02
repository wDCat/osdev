//
// Created by dcat on 8/31/17.
//

#ifndef W2_EXEC_H
#define W2_EXEC_H

#include "../../ker/include/system.h"
#include "proc.h"

int kexec(pid_t pid, const char *path, int argc, ...);

#endif //W2_EXEC_H
