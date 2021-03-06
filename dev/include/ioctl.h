//
// Created by dcat on 11/20/17.
//

#ifndef W2_IOCTL_H
#define W2_IOCTL_H

struct winsize {
    unsigned short ws_row;    /* rows, in characters */
    unsigned short ws_col;    /* columns, in characters */
    unsigned short ws_xpixel;    /* horizontal size, pixels */
    unsigned short ws_ypixel;    /* vertical size, pixels */
};

int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);

#endif //W2_IOCTL_H
