//
// Created by dcat on 4/3/17.
//

#include "include/catrfmt.h"
#include <str.h>
#include <vfs.h>
#include "include/catrfmt_def.h"
#include "include/catrfmt.h"

catrfmt_t *catrfmt_init(uint32_t mem_addr) {
    catrfmt_t *fs = (catrfmt_t *) kmalloc(sizeof(catrfmt_t));
    fs->header = (catrfmt_raw_header_t *) mem_addr;
    if (fs->header->magic != CATRFMT_MAGIC) {
        putf_const("[%x][%x][%x]", fs->header, fs->header->magic, CATRFMT_MAGIC);
        for (;;);
    }
    ASSERT(fs->header->magic == CATRFMT_MAGIC);
    fs->tables = (catrfmt_obj_table_t *) (mem_addr + (uint32_t) sizeof(catrfmt_raw_header_t));
    fs->table_count = fs->header->obj_table_count;

    dprintf("[CATRFMT]size:%x table_count:%x", fs->header->length, fs->header->obj_table_count);
    uint32_t *data_magic_a = (uint32_t *) (mem_addr + sizeof(catrfmt_raw_header_t) +
                                           fs->table_count * sizeof(catrfmt_obj_table_t));
    uint32_t data_magic = *data_magic_a;
    ASSERT(data_magic == CATRFMT_DATA_START_MAGIC);
    fs->data_addr = (uint32_t) ((uint32_t) data_magic_a + sizeof(uint32_t));
    for (int cur_table_no = 0; cur_table_no < fs->table_count; cur_table_no++) {
        catrfmt_obj_table_t *table = fs->tables + sizeof(catrfmt_obj_table_t) * cur_table_no;
        for (int x = 0; x < table->count; x++) {
            table->objects[x].fs = fs;
        }
    }
    return fs;
}

uint32_t catrfmt_read_inner(catrfmt_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff) {
    catrfmt_t *fs = fs_obj->fs;
    uint32_t saddr = fs->data_addr + fs_obj->offset + offset;
    dprintf("Reading %s data:%x offset:%x", fs_obj->name, fs->data_addr, saddr);
    int ret = 0;
    if (offset >= fs_obj->length) {
        return ret;
    } else if (offset + size > fs_obj->length) {
        ret = (uint32_t) memcpy(buff, saddr, fs_obj->length - offset);
    } else
        ret = (uint32_t) memcpy(buff, saddr, size);
    dprintf("ret:%x", ret);
    return ret;
}

uint32_t catrfmt_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {
    catrfmt_obj_t *fs_obj = (catrfmt_obj_t *) node->inode;
    return catrfmt_read_inner(fs_obj, offset, size, buff);
}

fs_node_t *catrfmt_create_fs_node(catrfmt_obj_t *fs_obj) {
    if (fs_obj->node) {
        return fs_obj->node;
    }
    fs_node_t *node = (fs_node_t *) kmalloc(sizeof(fs_node_t));
    memset(node, 0, sizeof(fs_node_t));
    memcpy(node->name, fs_obj->name, 256);
    node->length = fs_obj->length;
    node->inode = (uint32_t) fs_obj;
    fs_obj->node = node;
    return node;
}

uint8_t catrfmt_findbyname(catrfmt_t *fs, char *fn, fs_node_t **fs_node_out) {
    for (int cur_table_no = 0; cur_table_no < fs->table_count; cur_table_no++) {
        catrfmt_obj_table_t *table = fs->tables + sizeof(catrfmt_obj_table_t) * cur_table_no;
        for (int x = 0; x < table->count; x++) {
            if (strstr(table->objects[x].name, fn) == 0) {
                *fs_node_out = catrfmt_create_fs_node(table->objects[x].name);
                return 1;
            }
        }
    }
    return 0;
}

uint32_t catrfmt_write_inner(catrfmt_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff) {
    PANIC("[CATRFMT]writing not supported.");
}

uint32_t catrfmt_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff) {
    catrfmt_obj_t *fs_obj = (catrfmt_obj_t *) node->inode;
    return catrfmt_write_inner(fs_obj, offset, size, buff);
}

void catrfmt_dumpfilelist(catrfmt_t *fs) {
    for (int cur_table_no = 0; cur_table_no < fs->table_count; cur_table_no++) {
        dprintf("table[%d]", cur_table_no);
        catrfmt_obj_table_t *table = fs->tables + sizeof(catrfmt_obj_table_t) * cur_table_no;
        for (int x = 0; x < table->count; x++) {
            catrfmt_obj_t *obj = &table->objects[x];
            dprintf("[*]file:%s len:%d offset:%d", obj->name, obj->length, obj->offset);
        }
        dprintf("table[%d] end.", cur_table_no);
    }
}