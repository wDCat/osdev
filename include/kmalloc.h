//
// Created by dcat on 3/14/17.
//

#ifndef W2_KMALLOC_H
#define W2_KMALLOC_H

#include "intdef.h"

extern uint32_t end;
 uint32_t heap_placement_addr;

void kmalloc_install();

uint32_t kmalloc_internal(uint32_t sz, bool align, uint32_t *phys);

#define kmalloc(sz) kmalloc_internal(sz,false,NULL)
#define kmalloc_a(sz) kmalloc_internal(sz,true,NULL)
#define kmalloc_p(sz, phy) kmalloc_internal(sz,false,phy)
#define kmalloc_ap(sz, phy) kmalloc_internal(sz,true,phy)

#endif //W2_KMALLOC_H
