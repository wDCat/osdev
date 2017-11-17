//
// Created by dcat on 11/17/17.
//

#include <umem.h>
#include "cpuid.h"

uint32_t cpuid_feature_info;
uint32_t cpuid_extended_feature_info;

void cpuid_install() {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0x01, &eax, &ebx, &cpuid_extended_feature_info, &cpuid_feature_info);
}