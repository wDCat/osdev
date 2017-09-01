##Go die.
cmake_minimum_required(VERSION 3.6)
project(W2)
set(CMAKE_C_FLAGS "")

include_directories(include)
include_directories(dev/include)
include_directories(fs/catmfs/include)
include_directories(fs/ext2/include)
include_directories(fs/catmfs/include)
include_directories(fs/vfs/include)
include_directories(proc/elf/include)
include_directories(proc/include)
FILE(GLOB MyCSources *.c
        fs/ext2/ext2.c
        fs/vfs/vfs.c
        fs/catmfs/*.c
        dev/*.c
        proc/elf/*.c
        proc/*.c)
ADD_EXECUTABLE(MyExecutable ${MyCSources})