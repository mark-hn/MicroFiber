cmake_minimum_required(VERSION 3.10)

project(MicroFiber)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR})

set(SOURCES
        src/microfiber.cpp
        src/interrupt_manager.cpp
        src/thread_manager.cpp
        src/schedulers/scheduler.cpp
        src/schedulers/rand_scheduler.cpp
        src/schedulers/fcfs_scheduler.cpp
        src/schedulers/prio_scheduler.cpp
        src/queue.cpp
)

add_executable(microfiber ${SOURCES})
