cmake_minimum_required(VERSION 2.6)

project(DIAMONDAPPARATUS)

set(SOURCES client.cpp server.cpp time.cpp data.cpp clinkage.cpp)

add_library(diamondapparatus ${SOURCES})
target_link_libraries(diamondapparatus -lpthread)


add_executable(diamond main.cpp)
target_link_libraries(diamond -lpthread diamondapparatus)


add_executable(test test.cpp)
target_link_libraries(test -lpthread diamondapparatus)
    
install(TARGETS diamond RUNTIME DESTINATION bin)
install(TARGETS diamondapparatus ARCHIVE DESTINATION lib)

file(GLOB incfiles "${DIAMONDAPPARATUS_SOURCE_DIR}/*.h")
install(FILES ${incfiles} DESTINATION "include/diamondapparatus"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
        

