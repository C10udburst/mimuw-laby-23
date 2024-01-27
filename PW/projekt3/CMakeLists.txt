cmake_minimum_required (VERSION 3.11)
project (MIMUW-PW-lockfree C)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)  # prefer -pthread to -lphtread
find_package (Threads REQUIRED)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

set(CMAKE_C_FLAGS "-Wall -Wextra -Wpedantic -Wno-sign-compare -Wno-unused-parameter -O3 -march=native -fno-exceptions")

add_compile_options(-Og -g)
add_link_options(-Og -g)

#add_compile_options(-fsanitize=thread -Og -g)
#add_link_options(-fsanitize=thread -Og -g)

#add_compile_options(-fsanitize=address -Og -g)
#add_link_options(-fsanitize=address -Og -g)

add_library(queues OBJECT SimpleQueue.c RingsQueue.c LLQueue.c BLQueue.c HazardPointer.c)
target_link_libraries(queues PRIVATE Threads::Threads atomic)

add_executable(simpleTester simpleTester.c)
target_link_libraries(simpleTester PRIVATE queues Threads::Threads atomic)

# for loop to generate executables for (one|multiple)_read_(one|multiple)_write(|_isempty)
foreach (read "one" "multiple")
    foreach (write "one" "multiple")
        foreach (isempty "" "_with_isempty")
            add_executable(${read}_read_${write}_write${isempty} ${read}_read_${write}_write${isempty}.c)
            target_link_libraries(${read}_read_${write}_write${isempty} PRIVATE queues Threads::Threads atomic)
        endforeach()
    endforeach()
endforeach()

file(GLOB_RECURSE TESTERS "tests/*.c")
add_executable(tester tester.c ${TESTERS})
target_link_libraries(tester PRIVATE queues Threads::Threads atomic)

# run tests with valgrind
add_custom_target(valgrind
        COMMAND valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --verbose --log-file=valgrind-out.txt $<TARGET_FILE:tester>
        DEPENDS tester
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running tests with valgrind"
        VERBATIM
)
