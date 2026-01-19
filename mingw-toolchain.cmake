# MinGW 交叉编译工具链配置
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 指定编译器
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# 静态链接配置 - 使程序不依赖外部 DLL
# 这些标志确保所有库都静态链接到可执行文件中
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -Wl,--allow-multiple-definition")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

# 强制使用静态库（如果有的话）
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
set(BUILD_SHARED_LIBS OFF)

# 指定目标环境根目录（查找库和头文件）
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# 调整查找策略
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Windows 特定的定义
add_definitions(-DWIN32 -D_WIN32 -D__WIN32__ -D_WINDOWS)