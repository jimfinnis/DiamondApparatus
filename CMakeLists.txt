cmake_minimum_required(VERSION 2.6)

project(diamondapparatus)

set(SOURCES client.cpp server.cpp main.cpp time.cpp )

add_executable(diamond ${SOURCES})
target_link_libraries(diamond -lpthread)
    
