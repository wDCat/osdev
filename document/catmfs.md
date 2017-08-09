# Cat Memory FS

## Memory struct
```
.header begin
32 catmfs magic const=0xCACAACAC;
32 length;
32 obj_table_count;
(obj_table_count*sizeof(camfs_obj_table_t)) obj_tables;
32 data start magic const=0xACCACCDD;
data....
.header end
```

## Others

一个table存32个文件信息,每32个分配一个table
table.count 代表table中已有文件个数(好像是废话//)