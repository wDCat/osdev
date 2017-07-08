//
// Created by dcat on 7/7/17.
//

#include "proc.h"

void create_user_stack(uint32_t start_addr, uint32_t size) {
    for (int x = 0; x < size; x += 0x1000) {
        alloc_frame(
                get_page(start_addr + x, true, current_dir),
                false,
                true);
    }
}