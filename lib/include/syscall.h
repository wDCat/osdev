//
// Created by dcat on 9/7/17.
//

#ifndef W2_SYSCALL_H
#define W2_SYSCALL_H

#include "stdint.h"
#include "uproc.h"
#include "uvfs.h"
#include "stat.h"
#include "syscallno.h"

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

_define_syscall0(helloworld, SYS_restart_syscall);

_define_syscall1(screen_print, SYS_screen_print, const char*);

_define_syscall1(delay, SYS_delay, uint32_t);

_define_syscall0(fork, SYS_fork);

_define_syscall0(getpid, SYS_getpid);

_define_syscall1(hello_switcher, SYS_hello_switcher, pid_t);

_define_syscall1(exit, SYS_exit, uint32_t);

_define_syscall2(open, SYS_open, const char*, uint8_t);

_define_syscall1(close, SYS_close, int8_t);

_define_syscall3(read, SYS_read, int8_t, int32_t, uchar_t*);

_define_syscall3(write, SYS_write, int8_t, int32_t, uchar_t*);

_define_syscall2(stat, SYS_stat, const char*, stat_t*);

_define_syscall3(ls, SYS_ls, const char*, dirent_t*, uint32_t);

_define_syscall4(exec, SYS_exec, const char*, int, char*const*, char*const*);

_define_syscall3(waitpid, SYS_waitpid, pid_t, int*, int);

_define_syscall2(kill, SYS_kill, pid_t, int);

_define_syscall2(access, SYS_access, const char*, int);

_define_syscall1(chdir, SYS_chdir, const char*);

_define_syscall2(getcwd, SYS_getcwd, char*, int);

_define_syscall3(lseek, SYS_lseek, int8_t, off_t, int);

_define_syscall1(malloc, SYS_malloc, uint32_t);

_define_syscall1(free, SYS_free, void*);

_define_syscall3(dup3, SYS_dup3, int, int, int);

_define_syscall3(readv, SYS_readv, int, const struct iovec*, int);

_define_syscall3(writev, SYS_writev, int, const struct iovec*, int);

#endif //W2_SYSCALL_H
