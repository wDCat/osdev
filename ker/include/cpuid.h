//
// Created by dcat on 11/17/17.
//

#ifndef W2_CPUID_H
#define W2_CPUID_H

#include "intdef.h"

#define cpuid(reg, eax, ebx, ecx, edx) ({\
    __asm__ volatile("cpuid"\
    : "=a" (*(eax)), "=b" (*(ebx)), "=c" (*(ecx)), "=d" (*(edx))\
    : "0" (reg));\
})
extern uint32_t cpuid_feature_info;
extern uint32_t cpuid_extended_feature_info;

void cpuid_install();

#endif //W2_CPUID_H
