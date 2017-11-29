//
// Created by dcat on 11/28/17.
//

#ifndef W2_TIME_H
#define W2_TIME_H
typedef long time_t;
typedef long suseconds_t;
struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};
#endif //W2_TIME_H
