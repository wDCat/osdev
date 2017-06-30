//
// Created by dcat on 4/3/17.
//

#include <catmfs.h>
#include "catmfs.h"

catmfs_t *catmfs_init(uint32_t mem_addr) {
    catmfs_t *fs = (catmfs_t *) kmalloc(sizeof(catmfs_t));
    fs->header = (catmfs_raw_header_t *) mem_addr;
    ASSERT(fs->header->magic == CATMFS_MAGIC);
    fs->tables = (catmfs_obj_table_t *) (mem_addr + (uint32_t) sizeof(catmfs_raw_header_t));
    fs->table_count = fs->header->obj_table_count;
    uint32_t data_magic = mem_addr + sizeof(catmfs_raw_header_t) + fs->table_count * sizeof(catmfs_obj_table_t);
    ASSERT(data_magic == CATMFS_DATA_START_MAGIC);
    fs->data_addr = (uint32_t) (&data_magic + sizeof(uint32_t));
    return fs;
}

uint32_t catmfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {
    catmfs_obj_t *fs_obj = (catmfs_obj_t *) node->inode;
    catmfs_t *fs = fs_obj->fs;
    uint32_t saddr = fs->data_addr + fs_obj->offset + offset;
    if (offset >= fs_obj->length) {
        return 0;
    } else if (offset + size > fs_obj->length) {
        return memcpy(buff, saddr, fs_obj->length - (offset + size));
    } else
        return memcpy(buff, saddr, size);
}

uint32_t catmfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {

}