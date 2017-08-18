##Go die.
cmake_minimum_required(VERSION 3.6)
project(W2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


include_directories(include)

include_directories(dev/include)
include_directories(fs/catmfs/include)
include_directories(fs/ext2/include)

FILE(GLOB MyCSources *.c)
FILE(GLOB MyCSources fs/ext2/*.c)
ADD_EXECUTABLE(MyExecutable ${MyCSources})
