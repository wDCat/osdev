//
// Created by dcat on 11/17/17.
//

#include <cpuid.h>
#include <system.h>
#include <str.h>
#include "random.h"

uint32_t rand_seed = 12345;

inline bool is_cpu_supported_rdrand() {
    return (bool) BIT_GET(cpuid_extended_feature_info, 30);
}

int rand() {
    if (is_cpu_supported_rdrand()) {
        uint32_t rand;
        __asm__ __volatile__("rdrand %%eax":"=a"(rand));
        return (int) rand;
    } else {
        extern uint32_t timer_count;
        uint32_t ret = rand_seed * 5 + 12345;
        rand_seed = ret;
        return (int) (ret % RAND_MAX);
    }
}

void srand(uint seed) {
    rand_seed = seed;
}