//
// Created by dcat on 4/3/17.
//

#include <catmfs.h>
#include <str.h>
#include <catmfs_def.h>
#include "catmfs.h"

catmfs_t *catmfs_init(uint32_t mem_addr) {
    catmfs_t *fs = (catmfs_t *) kmalloc(sizeof(catmfs_t));
    fs->header = (catmfs_raw_header_t *) mem_addr;
    ASSERT(fs->header->magic == CATMFS_MAGIC);
    fs->tables = (catmfs_obj_table_t *) (mem_addr + (uint32_t) sizeof(catmfs_raw_header_t));
    fs->table_count = fs->header->obj_table_count;
    putf(STR("[CATMFS]size:%x table_count:%x\n"), fs->header->length, fs->header->obj_table_count);
    uint32_t data_magic = *(uint32_t *) (mem_addr + sizeof(catmfs_raw_header_t) +
                                         fs->table_count * sizeof(catmfs_obj_table_t));
    dumphex("data magic:", data_magic);
    ASSERT(data_magic == CATMFS_DATA_START_MAGIC);
    fs->data_addr = (uint32_t) (&data_magic + sizeof(uint32_t));
    return fs;
}

uint32_t catmfs_read_inner(catmfs_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff) {
    catmfs_t *fs = fs_obj->fs;
    uint32_t saddr = fs->data_addr + fs_obj->offset + offset;
    if (offset >= fs_obj->length) {
        return 0;
    } else if (offset + size > fs_obj->length) {
        return (uint32_t) memcpy(buff, saddr, fs_obj->length - (offset + size));
    } else
        return (uint32_t) memcpy(buff, saddr, size);
}

uint32_t catmfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {
    catmfs_obj_t *fs_obj = (catmfs_obj_t *) node->inode;
    return catmfs_read_inner(fs_obj, offset, size, buff);
}

fs_node_t *catmfs_create_fs_node(catmfs_obj_t *fs_obj) {
    if (fs_obj->node) {
        //putf(STR("use old node:%s\n"), fs_obj->name);
        return fs_obj->node;
    }
    //putf(STR("create new node:%s\n"), fs_obj->name);
    fs_node_t *node = (fs_node_t *) kmalloc(sizeof(fs_node_t));
    memset(node, 0, sizeof(fs_node_t));
    memcpy(node->name, fs_obj->name, 256);
    node->length = fs_obj->length;
    node->inode = (uint32_t) fs_obj;
    node->read = catmfs_read;
    node->write = catmfs_write;
    fs_obj->node = node;
    return node;
}

uint8_t catmfs_findbyname(catmfs_t *fs, char *fn, fs_node_t **fs_node_out) {
    for (int cur_table_no = 0; cur_table_no < fs->table_count; cur_table_no++) {
        catmfs_obj_table_t *table = fs->tables + sizeof(catmfs_obj_table_t) * cur_table_no;
        for (int x = 0; x < table->count; x++) {
            if (strstr(table->objects[x].name, fn) == 0) {
                *fs_node_out = catmfs_create_fs_node(table->objects[x].name);
                return 1;
            }
        }
    }
    return 0;
}

uint32_t catmfs_write_inner(catmfs_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff) {
    PANIC("[CATMFS]writing not supported.");
}

uint32_t catmfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {
    catmfs_obj_t *fs_obj = (catmfs_obj_t *) node->inode;
    return catmfs_write_inner(fs_obj, offset, size, buff);
}

void catmfs_dumpfilelist(catmfs_t *fs) {
    for (int cur_table_no = 0; cur_table_no < fs->table_count; cur_table_no++) {
        putf(STR("table[%d]\n"), cur_table_no);
        catmfs_obj_table_t *table = fs->tables + sizeof(catmfs_obj_table_t) * cur_table_no;
        for (int x = 0; x < table->count; x++) {
            catmfs_obj_t *obj = &table->objects[x];
            putf(STR("[*]file:%s len:%d offset:%d\n"), obj->name, obj->length, obj->offset);
        }
        putf(STR("table[%d] end.\n"), cur_table_no);
    }
}