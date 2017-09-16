//
// Created by dcat on 9/15/17.
//

#include "file.h"

FILE stdin_obj = {.fd=0}, stdout_obj = {.fd=1}, stderr_obj = {.fd=2};
FILE *stdin = &stdin_obj;
FILE *stdout = &stdout_obj;
FILE *stderr = &stderr_obj;