//
// Created by dcat on 3/12/17.
//

#ifndef W2_ISRS_H
#define W2_ISRS_H

#include "system.h"

/*Exception #	Description	Error Code?
0	Division By Zero Exception	No
1	Debug Exception	No
2	Non Maskable Interrupt Exception	No
3	Breakpoint Exception	No
4	Into Detected Overflow Exception	No
5	Out of Bounds Exception	No
6	Invalid Opcode Exception	No
7	No Coprocessor Exception	No
8	Double Fault Exception	Yes
9	Coprocessor Segment Overrun Exception	No
10	Bad TSS Exception	Yes
11	Segment Not Present Exception	Yes
12	Stack Fault Exception	Yes
13	General Protection Fault Exception	Yes
14	Page Fault Exception	Yes
15	Unknown Interrupt Exception	No
16	Coprocessor Fault Exception	No
17	Alignment Check Exception (486+)	No
18	Machine Check Exception (Pentium/586+)	No
19 to 31	Reserved Exceptions	No
*/
extern void _isr0();

extern void _isr1();

extern void _isr2();

extern void _isr3();

extern void _isr4();

extern void _isr5();

extern void _isr6();

extern void _isr7();

extern void _isr8();

extern void _isr9();

extern void _isr10();

extern void _isr11();

extern void _isr10();

extern void _isr11();

extern void _isr10();

extern void _isr11();

extern void _isr12();

extern void _isr13();

extern void _isr14();

extern void _isr15();

extern void _isr16();

extern void _isr17();

extern void _isr18();

extern void _isr19();

void fault_handler(struct regs *r);

void isrs_install();

#endif //W2_ISRS_H
