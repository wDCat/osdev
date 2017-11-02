//
// Created by dcat on 9/7/17.
//

#ifndef W2_SYSCALL_H
#define W2_SYSCALL_H

#include "stdint.h"
#include "uproc.h"
#include "uvfs.h"
#include "stat.h"

#define _define_syscall0(fn, num) long syscall_##fn()
#define _define_syscall1(fn, num, P1) long syscall_##fn(P1 p1)
#define _define_syscall2(fn, num, P1, P2) long syscall_##fn(P1 p1, P2 p2)
#define _define_syscall3(fn, num, P1, P2, P3) long syscall_##fn(P1 p1, P2 p2, P3 p3)
#define _define_syscall4(fn, num, P1, P2, P3, P4) long syscall_##fn(P1 p1, P2 p2, P3 p3,P4 p4)
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
#define _impl_syscall4(fn, num, P1, P2, P3, P4) \
long syscall_##fn(P1 p1, P2 p2, P3 p3,P4 p4) \
{ \
  long a; \
  __asm__ __volatile__("int $0x60" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3),"S"((int)p4)); \
  return a; \
}

_define_syscall0(helloworld, 0);

_define_syscall1(screen_print, 1, const char*);

_define_syscall1(delay, 2, uint32_t);

_define_syscall0(fork, 3);

_define_syscall0(getpid, 4);

_define_syscall1(hello_switcher, 5, pid_t);

_define_syscall1(exit, 6, uint32_t);

_define_syscall2(open, 7, const char*, uint8_t);

_define_syscall1(close, 8, int8_t);

_define_syscall3(read, 9, int8_t, int32_t, uchar_t*);

_define_syscall3(write, 10, int8_t, int32_t, uchar_t*);

_define_syscall2(stat, 11, const char*, stat_t*);

_define_syscall3(ls, 12, const char*, dirent_t*, uint32_t);

_define_syscall4(exec, 13, const char*, int, char*const*, char*const*);

_define_syscall3(waitpid, 14, pid_t, int*, int);

_define_syscall2(kill, 15, pid_t, int);

_define_syscall2(access, 16, const char*, int);

_define_syscall1(chdir, 17, const char*);

_define_syscall2(getcwd, 18, char*, int);

_define_syscall3(lseek, 19, int8_t, off_t, int);

_define_syscall1(malloc, 20, uint32_t);

_define_syscall1(free, 21, void*);

_define_syscall3(dup3, 22, int, int, int);

#endif //W2_SYSCALL_H
