//
// Created by dcat on 11/7/17.
//
#ifndef W2_SYSCALLNO_H
#define W2_SYSCALLNO_H

#define SYS_restart_syscall 0
#define SYS_screen_print 1
#define SYS_delay 2
#define SYS_fork 3
#define SYS_getpid 4
#define SYS_hello_switcher 5
#define SYS_exit 6
#define SYS_open 7
#define SYS_close 8
#define SYS_read 9
#define SYS_write 10
#define SYS_stat 11
#define SYS_lstat 11
#define SYS_fstat 11
#define SYS_ls 12
#define SYS_exec 13
#define SYS_waitpid 14
#define SYS_kill 15
#define SYS_access 16
#define SYS_chdir 17
#define SYS_getcwd 18
#define SYS_lseek 19
#define SYS_malloc 20
#define SYS_free 21
#define SYS_dup3 22
#define SYS_readv 23
#define SYS_writev 24
#define SYS_stat64 25
#define SYS_lstat64 25
#define SYS_fstat64 25
#define SYS_ioctl 26
#define SYS_brk 27
#define SYS_execve 28
#define SYS_signal 29
#define SYS_sigaction 30
#define SYS_rt_sigaction 30
#define SYS_wait4 31
#define SYS_fcntl 32
#define SYS_getdents 33
#define SYS_getdents64 33
#undef SYS_getdents64
#endif //W2_SYSCALLNO_H
