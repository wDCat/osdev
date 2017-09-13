//
// Created by dcat on 9/13/17.
//

#ifndef W2_WAIT_H
#define W2_WAIT_H

#include "proc.h"

pid_t sys_waitpid(pid_t pid, int *status, int options);

#endif //W2_WAIT_H
