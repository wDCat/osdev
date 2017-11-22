//
// Created by dcat on 9/7/17.
//


#include "syscall.h"

_impl_syscall0(helloworld, SYS_restart_syscall);

_impl_syscall1(screen_print, SYS_screen_print, const char*);

_impl_syscall1(delay, SYS_delay, uint32_t);

_impl_syscall0(fork, SYS_fork);

_impl_syscall0(getpid, SYS_getpid);

_impl_syscall1(hello_switcher, SYS_hello_switcher, pid_t);

_impl_syscall1(exit, SYS_exit, uint32_t);

_impl_syscall2(open, SYS_open, const char*, int);

_impl_syscall1(close, SYS_close, int8_t);

_impl_syscall3(read, SYS_read, int8_t, uchar_t*, int32_t);

_impl_syscall3(write, SYS_write, int8_t, uchar_t*, int32_t);

_impl_syscall2(stat, SYS_stat, const char*, stat_t*);

_impl_syscall3(ls, SYS_ls, const char*, dirent_t*, uint32_t);

_impl_syscall4(exec, SYS_exec, const char*, int, char*const*, char*const*);

_impl_syscall3(waitpid, SYS_waitpid, pid_t, int*, int);

_impl_syscall2(kill, SYS_kill, pid_t, int);

_impl_syscall2(access, SYS_access, const char*, int);

_impl_syscall1(chdir, SYS_chdir, const char*);

_impl_syscall2(getcwd, SYS_getcwd, char*, int);

_impl_syscall3(lseek, SYS_lseek, int8_t, off_t, int);

_impl_syscall1(malloc, SYS_malloc, uint32_t);

_impl_syscall1(free, SYS_free, void*);

_impl_syscall3(dup3, SYS_dup3, int, int, int);

_impl_syscall3(readv, SYS_readv, int, void*, int);

_impl_syscall3(writev, SYS_writev, int, void*, int);

_impl_syscall3(ioctl, SYS_ioctl, unsigned long, unsigned long, unsigned long);

_impl_syscall1(brk, SYS_brk, void*);