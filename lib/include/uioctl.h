//
// Created by dcat on 11/22/17.
//

#ifndef W2_UIOCTL_H
#define W2_UIOCTL_H
#define TIOCGWINSZ    0x5413
struct winsize {
    unsigned short ws_row;    /* rows, in characters */
    unsigned short ws_col;    /* columns, in characters */
    unsigned short ws_xpixel;    /* horizontal size, pixels */
    unsigned short ws_ypixel;    /* vertical size, pixels */
};
#endif //W2_UIOCTL_H
