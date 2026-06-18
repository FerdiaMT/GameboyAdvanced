
cmake_minimum_required(VERSION 3.10)
project(GBA)

set(CMAKE_CXX_STANDARD 17)

add_executable(GBA Bus.cpp CPU.cpp DebuggerCPU.cpp GBA.cpp Main.cpp PPU.cpp)
target_include_directories(GBA PRIVATE include/)