# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0.0)
# 项目信息
project(net VERSION 0.1.0)

# 添加头文件
include_directories(./disk.h ./naiveExt2.h)
# 查找当前目录下的所有源文件，并将ls名称保存到DIR_SRCS变量
aux_source_directory(./ DIR_SRCS)
# 指定生成目标
add_executable(main ${DIR_SRCS})
