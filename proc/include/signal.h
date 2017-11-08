//
// Created by dcat on 9/14/17.
//

#ifndef W2_SIGNAL_H
#define W2_SIGNAL_H

#include <system.h>
#include "proc.h"

#define SIG_COUNT 22

#define SIGHUP         1
#define SIGINT         2
#define SIGQUIT         3
#define SIGILL         4
#define SIGTRAP         5
#define SIGABRT         6
#define SIGIOT         6
#define SIGUNUSED     7
#define SIGFPE         8
#define SIGKILL         9
#define SIGUSR1        10
#define SIGSEGV        11
#define SIGUSR2        12
#define SIGPIPE        13
#define SIGALRM        14
#define SIGTERM        15
#define SIGSTKFLT    16
#define SIGCHLD        17
#define SIGCONT        18
#define SIGSTOP        19
#define SIGTSTP        20
#define SIGTTIN        21
#define SIGTTOU        22
//Linux like?
#define SIG_SET(obj, sig) obj|=(1<<(sig-1))
#define SIG_CLEAR(obj, sig) obj&=~(1<<(sig-1))
#define SIG_TEST(obj, sig) (obj&(1<<(sig-1)))

int do_signal(pcb_t *pcb, regs_t *r);

void send_sig(pcb_t *pcb, int sig);

int sys_kill(pid_t pid, int sig);

void signal_print_proc(pcb_t *pcb);

#endif //W2_SIGNAL_H
