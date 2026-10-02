cmake_minimum_required(VERSION 3.10)
# 顶层 project():让 ${PROJECT_SOURCE_DIR} 等变量指向本工程根目录;
# apl_lvgl/CMakeLists.txt 已加 CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR
# 检测,作为 subdirectory 引用时会自动跳过自己的 project(),避免冲突。
project(lvgl C CXX)

# ============================================================================
# ★ 用户配置区：整个仿真器只认下面这 8 项，其余都在文件下面
# 换工程改第 1 行；换屏幕改 SIM_H_RES / SIM_V_RES。全部普通 set，命令行 -D 压不过。
# ============================================================================
set(TARGET_PROJ_DIR "D:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/p4_touch_lcd4_3_exp/lvgl_demo_ai/eez-test") # 目标工程根，其余路径都跟它拼
set(APL_LVGL_DIR "D:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/common/APL/apl_lvgl_v9_4")                     # LVGL 内核全路径（手填，不做自动查找）
set(SIM_H_RES 800)                                                                                                                # 仿真窗口宽 = 实际 LCD 宽
set(SIM_V_RES 480)                                                                                                                # 仿真窗口高 = 实际 LCD 高

set(LVGL_CONF_FILE "${TARGET_PROJ_DIR}/design/sim_cmake/lv_conf.h")                                                               # 仿真专用 lv_conf.h
set(UI_DIR "${TARGET_PROJ_DIR}/src/ui")                                                                                           # EEZ 生成代码（平铺 .c/.cpp）
set(NATIVE_DIR "${TARGET_PROJ_DIR}/src/native")                                                                                    # 用户逻辑（排除 io_esp / test_native）
set(SIM_MAIN_DIR "${TARGET_PROJ_DIR}/design/sim_cmake")                                                                            # 仿真入口 main.c + lv_conf.h
# ============================================================================

# 界面源文件：EEZ 平铺布局（src/ui 下直接是 .c/.cpp，不是 lv_ui/ 子目录）
file(GLOB_RECURSE UI_SOURCES
     "${UI_DIR}/*.c"
     "${UI_DIR}/*.cpp")
# 用户代码层：不收会链接期 undefined reference to get_var_*/set_var_*；排除 io_esp / test_native
file(GLOB_RECURSE NATIVE_SOURCES
     "${NATIVE_DIR}/*.c"
     "${NATIVE_DIR}/*.cpp")
list(FILTER NATIVE_SOURCES EXCLUDE REGEX "(io_esp|test_native)\\.(c|cpp)$")

# 定义后 lv_interface_sim.c 才产出仿真数据；硬件工程不定义，由硬件后端接管
option(LV_INTERFACE_SIM "Enable LVGL simulation data source (PC simulator)" ON)

# 内核有效性检查
if(NOT EXISTS "${APL_LVGL_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "[sim] 配置区 APL_LVGL_DIR 无效: ${APL_LVGL_DIR}\n"
        "      请在上面配置区写 set(APL_LVGL_DIR \"<到 apl_lvgl 的全路径>\")")
endif()

# Set the correct FreeRTOS port for your system (e.g., Posix for WSL)
set(FREERTOS_PORT GCC_POSIX CACHE STRING "Port for FreeRTOS on Posix environment")

unset(USE_FREERTOS CACHE)

option(USE_FREERTOS "Enable FreeRTOS" OFF) # Turn this on to enable FreeRTOS

if(USE_FREERTOS)
    message(STATUS "FreeRTOS is enabled")

    # FreeRTOS integration when USE_FREERTOS is enabled
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/config)
    target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)

    # Add FreeRTOS as a subdirectory
    add_subdirectory(FreeRTOS)

    # FreeRTOS-specific include directories
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/include)
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix)
    include_directories(${PROJECT_SOURCE_DIR}/config)

    # Add FreeRTOS sources
    file(GLOB FREERTOS_SOURCES
        "${PROJECT_SOURCE_DIR}/FreeRTOS/*.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/MemMang/heap_4.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix/*.c"
    )
else()
    message(STATUS "FreeRTOS is disabled")
    set(FREERTOS_SOURCES "")  # No FreeRTOS sources if FreeRTOS is disabled
endif()

# Main include files of the project
include_directories(${PROJECT_SOURCE_DIR}/main/inc)

# Define options for LVGL with default values (OFF)
option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
option(LV_USE_FREETYPE "Use freetype library" OFF)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output path for the executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# ccache support: caches compiler output to speed up rebuilds
# Falls back to direct compiler call if ccache is not found
# ----------------------------------------------------------------------------
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache found, enabling compiler cache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
    message(STATUS "ccache not found, using direct compiler (no caching)")
endif()

# Find and include SDL2 library
find_package(SDL2 REQUIRED)

# Remove ARM-specific compile and linker options
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

# Add compile definitions based on the selected options
add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBPNG}>:LV_USE_LIBPNG=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBJPEG_TURBO}>:LV_USE_LIBJPEG_TURBO=1>)
add_compile_definitions($<$<BOOL:${LV_USE_FFMPEG}>:LV_USE_FFMPEG=1>)
add_compile_definitions($<$<BOOL:${LV_INTERFACE_SIM}>:LV_INTERFACE_SIM=1>)


# 直接指向共享组件 apl_lvgl:它会跳过自己的 project()(已在自身 CMakeLists
# 加顶层检测),通过 env_support/cmake/custom.cmake 产出
# lvgl / lvgl_examples / lvgl_demos / lvgl_thorvg 四个库供本工程链接
# 复用上面推导/手填的内核路径;写回 CACHE 是因为 apl_lvgl 是 add_subdirectory 子作用域,
# 父作用域的普通变量传不进去(子作用域只读得到 CACHE)
set(APL_LVGL_DIR "${APL_LVGL_DIR}" CACHE PATH "Shared LVGL component")
# 把 LVGL_CONF_FILE 注进内核（apl_lvgl 是子作用域，普通变量传不进去 → 必须 CACHE，
# 否则内核静默回落自带 lv_conf：ThorVG=1/FS_STDIO=1，PC 上编不过）；_DIR 置空，
# 与 _PATH 并存内核会 FATAL。详见 P-0066
set(LV_BUILD_CONF_DIR "" CACHE PATH "置空：与 _PATH 并存 apl_lvgl 会 FATAL")
set(LV_BUILD_CONF_PATH "${LVGL_CONF_FILE}" CACHE PATH "Shared LVGL config header")
set(LV_CONF_PATH "${LVGL_CONF_FILE}" CACHE PATH "Shared LVGL config header")
# 内核内置 ThorVG 是条 option、默认 ON（与 lv_conf.h 里的 0 无关），必须在下面
# add_subdirectory 之前关掉，否则 PC 上编不过。详见 P-0076
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "关掉内核内置的 ThorVG(PC 上编不过)")
add_subdirectory(${APL_LVGL_DIR} ${CMAKE_BINARY_DIR}/apl_lvgl_build)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})
# lvgl 用 file(GLOB_RECURSE src/*.c src/*.cpp) 把 src/libs/thorvg/*.cpp 一并收进
# lvgl target,而 thorvg 要的 config.h 只有它自己的构建才生成(全树都没有)→ 编必挂。
# 本 UI 纯控件+内嵌图,不需要矢量图形,把这些源文件改成"只当头文件"跳过编译。
# ★ 正则必须两种斜杠都吃:Windows 上 SOURCES 是反斜杠路径,写死 "libs/thorvg"
#   会静默匹配不上、thorvg 照编照挂(P-0076)
get_target_property(_lv_srcs lvgl SOURCES)
set(_blk 0)
foreach(_s IN LISTS _lv_srcs)
    if(_s MATCHES "libs[\\/]thorvg" OR _s MATCHES "vg_lite_tvg")
        set_source_files_properties(${_s} PROPERTIES HEADER_FILE_ONLY TRUE)
        math(EXPR _blk "${_blk}+1")
    endif()
endforeach()
message(STATUS "[sim] 屏蔽 ThorVG/vg_lite 源码 ${_blk} 个")
message(STATUS "[sim] 工程=${TARGET_PROJ_DIR}")
message(STATUS "[sim]   窗口=${SIM_H_RES}x${SIM_V_RES}  内核=${APL_LVGL_DIR}")
message(STATUS "[sim]   UI=${UI_DIR}")

# Create the main executable, depending on the FreeRTOS option
if(USE_FREERTOS)
    add_executable(main
        ${PROJECT_SOURCE_DIR}/main/src/main.c
        ${PROJECT_SOURCE_DIR}/main/src/freertos_main.cpp
        ${PROJECT_SOURCE_DIR}/main/src/mouse_cursor_icon.c
        ${PROJECT_SOURCE_DIR}/main/src/FreeRTOS_Posix_Port.c
        ${FREERTOS_SOURCES}  # Add only if USE_FREERTOS is enabled
        ${UI_SOURCES}
    )
    # Link FreeRTOS libraries
    target_link_libraries(main freertos_config FreeRTOS)
else()
    add_executable(main
        ${SIM_MAIN_DIR}/main.c                             # 工程侧仿真入口(引用,不复制)
        ${PROJECT_SOURCE_DIR}/main/src/mouse_cursor_icon.c  # 仿真器自带光标位图
        ${UI_SOURCES}
        ${NATIVE_SOURCES}                                  # 工程用户代码层 + io_pc
    )
endif()

# Define LVGL configuration as a simple include
target_compile_definitions(main PRIVATE LV_CONF_INCLUDE_SIMPLE)
# m/pthread 是 POSIX 库名,Windows 会翻成 m.lib/pthread.lib(根本不存在)→ 链接直接挂
if(WIN32)
    target_link_libraries(main lvgl ${SDL2_LIBRARIES})
else()
    target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} m pthread)
endif()
target_include_directories(main PRIVATE
    ${UI_DIR}
    ${NATIVE_DIR}
    ${SIM_MAIN_DIR})

# Only link freertos_config if the FreeRTOS directory exists
if(USE_FREERTOS)
    target_link_libraries(main freertos_config)
endif()

# Custom target to run the executable
add_custom_target(run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)

# Conditionally include and link SDL2_image if LV_USE_DRAW_SDL is enabled
if(LV_USE_DRAW_SDL)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    find_package(SDL2_image REQUIRED)
    target_include_directories(lvgl PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(main ${SDL2_IMAGE_LIBRARIES})
endif()

# Conditionally include and link libpng if LV_USE_LIBPNG is enabled
if(LV_USE_LIBPNG)
    find_package(PNG REQUIRED)
    target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIRS})
    target_link_libraries(main ${PNG_LIBRARIES})
endif()

# Conditionally include and link libjpeg-turbo if LV_USE_LIBJPEG_TURBO is enabled
if(LV_USE_LIBJPEG_TURBO)
    find_package(JPEG REQUIRED)
    target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
    target_link_libraries(main ${JPEG_LIBRARIES})
endif()

# Conditionally include and link FFmpeg libraries if LV_USE_FFMPEG is enabled
if(LV_USE_FFMPEG)
    target_link_libraries(main avformat avcodec avutil swscale z)
endif()

# Conditionally include and link FreeType if LV_USE_FREETYPE is enabled
if(LV_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(main ${FREETYPE_LIBRARIES})
endif()

# Apply additional compile options if the build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode enabled")

    target_compile_options(lvgl PRIVATE
        -pedantic-errors
        -Wall
        -Wclobbered
        -Wdeprecated
        -Wdouble-promotion
        -Wempty-body
        -Wextra
        -Wformat-security
        -Wmaybe-uninitialized
        # -Wmissing-prototypes
        -Wpointer-arith
        -Wmultichar
        -Wno-pedantic # ignored for now, we convert functions to pointers for properties table.
        -Wreturn-type
        -Wshadow
        -Wshift-negative-value
        -Wsizeof-pointer-memaccess
        -Wtype-limits
        -Wundef
        -Wuninitialized
        -Wunreachable-code
        -Wfloat-conversion
        -Wstrict-aliasing
    )

if (ASAN)
    message(STATUS "AddressSanitizer enabled")

    # Add AddressSanitizer flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
else()
    message(STATUS "AddressSanitizer disabled")
endif()
endif()
