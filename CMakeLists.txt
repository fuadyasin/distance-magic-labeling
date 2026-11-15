cmake_minimum_required (VERSION 3.10)

project ("DML" LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

# The search is CPU bound, so default to an optimised build.
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set (CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif ()

find_package (Threads REQUIRED)

add_library (labeling STATIC "graph.cpp" "graph.h" "labeling.cpp" "labeling.h" "cli.cpp" "cli.h")
target_include_directories (labeling PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries (labeling PUBLIC Threads::Threads)
if (MSVC)
	target_compile_options (labeling PRIVATE /W4)
else ()
	target_compile_options (labeling PRIVATE -Wall -Wextra)
endif ()

add_executable (DML "DML.cpp")
add_executable (DAML "DAML.cpp")
add_executable (CDML "CDML.cpp")
target_link_libraries (DML PRIVATE labeling)
target_link_libraries (DAML PRIVATE labeling)
target_link_libraries (CDML PRIVATE labeling)

enable_testing ()
add_executable (labeling_tests "tests.cpp")
target_link_libraries (labeling_tests PRIVATE labeling)
add_test (NAME labeling_tests COMMAND labeling_tests)
