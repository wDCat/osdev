##Go die.
cmake_minimum_required(VERSION 3.6)
project(W2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


include_directories(include)

include_directories(dev/include)
include_directories(fs/catmfs/include)
include_directories(fs/ext2/include)
include_directories(fs/catmfs/include)
include_directories(fs/vfs/include)

FILE(GLOB MyCSources *.c
        fs/ext2/ext2.c
        fs/vfs/vfs.c
        fs/catmfs/*.c
        dev/*.c)
ADD_EXECUTABLE(MyExecutable ${MyCSources})
