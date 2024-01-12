cmake_minimum_required (VERSION 3.5)
project (MIMUW-PW-lab11 C)

find_package (Threads REQUIRED)

# Set the C17 standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags for GCC targeting C17
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -std=c17 -latomic")

# http://stackoverflow.com/questions/10555706/
macro (add_executable _name)
    # invoke built-in add_executable
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(${_name}  PUBLIC atomic)
    endif()
endmacro()

#add_executable(mutual-excl mutual-excl.c)
#add_executable(atomic-volatile atomic-volatile.c)
#add_executable(lockfree-stack lockfree-stack.c)
add_executable(dekker dekker.c)

install(TARGETS DESTINATION .)
