cmake_minimum_required(VERSION 2.8)
project(elevator)

add_definitions(-std=c++11)

set(SOURCES
    main.cpp
    elevator.cpp
    )

set(HEADERS
    elevator.h
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(
    ${PROJECT_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    pthread
    )
