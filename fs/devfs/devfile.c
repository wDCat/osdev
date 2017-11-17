//
// Created by dcat on 11/17/17.
//

#include <catmfs.h>
#include <devfs.h>
#include <str.h>
#include <random.h>
#include "devfile.h"

int32_t devnull_oper() {
    return 0;
}

file_operations_t devnullops = {
        .read=(read_type_t) devnull_oper,
        .write=(write_type_t) devnull_oper,
        .lseek=(lseek_type_t) devnull_oper,
        .open=(open_type_t) devnull_oper,
        .close=(close_type_t) devnull_oper
};

int32_t devrandom_read(fs_node_t *node, __fs_special_t *fsp_, uint32_t size, uint8_t *buff) {
    extern uint32_t timer_count;
    for (uint32_t x = 0; x < size / 2; x += 2) {
        int randnum = rand();
        buff[x] = (uint8_t) (randnum % 0xFF);
        buff[x + 1] = (uint8_t) ((randnum >> 8) % 0xFF);
    }
    return size;
}

file_operations_t devrandomops = {
        .read=devrandom_read
};

int devfile_register_self() {
    srand(1024);
    devfs_register_dev("null", &devnullops, NULL);
    devfs_register_dev("random", &devrandomops, NULL);
    return 0;
}