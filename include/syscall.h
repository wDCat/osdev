//
// Created by dcat on 7/7/17.
//

#ifndef W2_SYSCALL_H
#define W2_SYSCALL_H

#include "isrs.h"
#include "../proc/elf/include/proc.h"

#define _define_syscall0(fn, num) long syscall_##fn()
#define _define_syscall1(fn, num, P1) long syscall_##fn(P1 p1)
#define _define_syscall2(fn, num, P1, P2) long syscall_##fn(P1 p1, P2 p2)
#define _define_syscall3(fn, num, P1, P2, P3) long syscall_##fn(P1 p1, P2 p2, P3 p3)
extern long errno;
#define _impl_syscall0(fn, num) \
long syscall_##fn() \
{ \
  long a; \
  __asm__ __volatile__("int $0x60" : "=a" (a) : "0" (num)); \
  return a; \
}

#define _impl_syscall1(fn, num, P1) \
long syscall_##fn(P1 p1) \
{ \
  long a; \
  __asm__ __volatile__("int $0x60" : "=a" (a) : "0" (num), "b" ((int)p1)); \
  return a; \
}

#define _impl_syscall2(fn, num, P1, P2) \
long syscall_##fn(P1 p1, P2 p2) \
{ \
  long a; \
  __asm__ __volatile__("int $0x60" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
  return a; \
}

#define _impl_syscall3(fn, num, P1, P2, P3) \
long syscall_##fn(P1 p1, P2 p2, P3 p3) \
{ \
  long a; \
  __asm__ __volatile__("int $0x60" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3)); \
  return a; \
}

_define_syscall0(helloworld, 0);

_define_syscall1(screen_print, 1, const char*);

_define_syscall1(delay, 2, uint32_t);

_define_syscall0(fork, 3);

_define_syscall0(getpid, 4);

_define_syscall1(hello_switcher, 5, pid_t);

_define_syscall1(exit, 6, uint32_t);
void syscall_install();

int syscall_handler(regs_t *r);

#endif //W2_SYSCALL_H
