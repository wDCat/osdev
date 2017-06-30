//
// Created by dcat on 3/20/17.
//

#include "fs_node.h"

uint32_t read_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {
    if (node->read == NULL)return -1;
    return node->read(node, offset, size, buff);
}

uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {
    if (node->write == NULL)return -1;
    return node->write(node, offset, size, buff);
}