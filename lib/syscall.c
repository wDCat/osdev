//
// Created by dcat on 9/7/17.
//


#include "syscall.h"

_impl_syscall0(helloworld, 0);

_impl_syscall1(screen_print, 1, const char*);

_impl_syscall1(delay, 2, uint32_t);

_impl_syscall0(fork, 3);

_impl_syscall0(getpid, 4);

_impl_syscall1(hello_switcher, 5, pid_t);

_impl_syscall1(exit, 6, uint32_t);

_impl_syscall2(open, 7, const char*, uint8_t);

_impl_syscall1(close, 8, int8_t);

_impl_syscall3(read, 9, int8_t, int32_t, uchar_t*);

_impl_syscall3(write, 10, int8_t, int32_t, uchar_t*);

_impl_syscall2(stat, 11, const char*, stat_t*);

_impl_syscall3(ls, 12, const char*, dirent_t*, uint32_t);

_impl_syscall2(exec, 13, const char*, int);

_impl_syscall3(waitpid, 14, pid_t, int*, int);

_impl_syscall2(kill, 15, pid_t, int);

_impl_syscall2(access, 16, const char*, int);