# CMakeList.txt: src 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("buaa_compiler")

include_directories(./ ./front ./ir ./gen ./util)

add_subdirectory(front)
add_subdirectory(gen)
add_subdirectory(ir)
add_subdirectory(util)

aux_source_directory(. DIR_SRCS)

# 将源代码添加到此项目的可执行文件。
add_executable (compiler ${DIR_SRCS})

target_link_libraries(compiler front gen ir util)
# TODO: 如有需要，请添加测试并安装目标。
SET(CMAKE_CXX_STANDARD 17)