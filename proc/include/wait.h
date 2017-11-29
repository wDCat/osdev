//
// Created by dcat on 9/13/17.
//

#ifndef W2_WAIT_H
#define W2_WAIT_H

#include "proc.h"
#include "time.h"

struct rusage {
    struct timeval ru_utime; /* user CPU time used */
    struct timeval ru_stime; /* system CPU time used */
    long ru_maxrss;        /* maximum resident set size */
    long ru_ixrss;         /* integral shared memory size */
    long ru_idrss;         /* integral unshared data size */
    long ru_isrss;         /* integral unshared stack size */
    long ru_minflt;        /* page reclaims (soft page faults) */
    long ru_majflt;        /* page faults (hard page faults) */
    long ru_nswap;         /* swaps */
    long ru_inblock;       /* block input operations */
    long ru_oublock;       /* block output operations */
    long ru_msgsnd;        /* IPC messages sent */
    long ru_msgrcv;        /* IPC messages received */
    long ru_nsignals;      /* signals received */
    long ru_nvcsw;         /* voluntary context switches */
    long ru_nivcsw;        /* involuntary context switches */
};

pid_t sys_waitpid(pid_t pid, int *status, int options);

pid_t sys_wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage);

#endif //W2_WAIT_H
