##Go die.
cmake_minimum_required(VERSION 3.6)
project(W2)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


include_directories(include)

FILE(GLOB MyCSources *.c)

ADD_EXECUTABLE(MyExecutable ${MyCSources})
