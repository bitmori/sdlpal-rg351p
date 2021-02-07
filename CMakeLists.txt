# CMake 最低版本号要求，你也可以设置版本范围，比如 3.1...3.15
cmake_minimum_required (VERSION 3.0)

# 项目信息，可设置版本号以及描述
project (sdlpal VERSION 0.1.0 DESCRIPTION "SDLPal")

# 防止在錯誤位置執行make污染源碼
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)


# ADD_SUBDIRECTORY 指令： 用于指定源码目录，并指定中间二进制和目标二进制存放的位置
ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (src/adplug)
ADD_SUBDIRECTORY (src/libmad)
ADD_SUBDIRECTORY (src/liboggvorbis/src)
ADD_SUBDIRECTORY (src/libopusfile/src)
ADD_SUBDIRECTORY (src/libopusfile/celt)
ADD_SUBDIRECTORY (src/libopusfile/silk)
ADD_SUBDIRECTORY (src/libopusfile/float)


# 指定头文件所在位置
# target_include_directories(demo_lib PUBLIC ${CMAKE_SOURCE_DIR}/include}

message("CMAKE_SOURCE_DIR= ${CMAKE_SOURCE_DIR}")
message("CMAKE_BINARY_DIR= ${CMAKE_BINARY_DIR}")

# 生成一个 demo 的可执行文件
add_executable(sdlpal main.cpp)