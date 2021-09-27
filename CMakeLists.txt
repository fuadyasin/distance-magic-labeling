# CMakeList.txt : CMake project for DML, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("DML")

# Add source to this project's executable.
add_executable (DML "DML.cpp" "DML.h"   "graph.cpp" "graph.h")
add_executable (DAML "DAML.cpp" "DML.h"   "graph.cpp" "graph.h")

# TODO: Add tests and install targets if needed.
