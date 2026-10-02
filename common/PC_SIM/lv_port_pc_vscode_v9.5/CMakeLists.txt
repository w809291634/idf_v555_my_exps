cmake_minimum_required(VERSION 3.10)
# 顶层 project():让 ${PROJECT_SOURCE_DIR} 等变量指向本工程根目录;
# apl_lvgl/CMakeLists.txt 已加 CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR
# 检测,作为 subdirectory 引用时会自动跳过自己的 project(),避免冲突。
project(lvgl C CXX)

# ============================================================================
# 用户配置区（PC 仿真器复用：改这里即可指向你自己的目标工程）
# ----------------------------------------------------------------------------
# 1) 共享 UI 层:所有界面位于目标工程的 main/ui,PC 仿真器直接编译该目录。
#    换工程时只需改这一行(指向该工程的 main/ui 目录)。
#    命令行覆盖示例: cmake -DUI_DIR=/path/to/xxx/main/ui ..
set(UI_DIR "${PROJECT_SOURCE_DIR}/../../../Template/spilcd_spiopt_lv_port_pc_tem_atks3/main/ui"
    CACHE PATH "Shared UI source dir (point to target project's main/ui)")

# 界面源文件收集:递归编译 lv_ui/(纯 LVGL 界面) + lv_interface/(数据交互契约)。
# 生成器产物(custom/guider/squareline)不参与编译,仅作参考。
file(GLOB_RECURSE UI_SOURCES
     "${UI_DIR}/lv_ui/*.c"
     "${UI_DIR}/lv_interface/*.c")

# 仿真器标志:定义后 lv_interface_sim.c 才会产出仿真数据(见该文件顶部注释)。
# 真实硬件工程不定义此宏,由硬件后端接管数据。
option(LV_INTERFACE_SIM "Enable LVGL simulation data source (PC simulator)" ON)

# 2) LVGL 配置:使用共享 UI 目录下的 lv_conf.h;分辨率/颜色深度在此适配
#    (若目标工程用 Kconfig 而非 lv_conf.h,可改 LV_CONF_PATH 指向对应文件)
set(LV_CONF_PATH "${UI_DIR}/lv_conf.h" CACHE PATH "Shared LVGL config header")

# 3) 共享 LVGL 组件 apl_lvgl(一般无需修改)
set(APL_LVGL_DIR "${PROJECT_SOURCE_DIR}/../../APL/apl_lvgl"
    CACHE PATH "Shared LVGL component (apl_lvgl)")

# 4) 仿真窗口分辨率:须与目标硬件 LCD 分辨率一致,保证两端同一布局。
#    以编译宏 SIM_H_RES / SIM_V_RES 传给 main.c,源码无需写死。
set(SIM_H_RES 320 CACHE STRING "Simulated window width (match target LCD)")
set(SIM_V_RES 240 CACHE STRING "Simulated window height (match target LCD)")
# ============================================================================

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
set(APL_LVGL_DIR "${PROJECT_SOURCE_DIR}/../../APL/apl_lvgl")
add_subdirectory(${APL_LVGL_DIR} ${CMAKE_BINARY_DIR}/apl_lvgl_build)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

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
        ${PROJECT_SOURCE_DIR}/main/src/main.c
        ${PROJECT_SOURCE_DIR}/main/src/mouse_cursor_icon.c
        ${UI_SOURCES}
    )
endif()

# Define LVGL configuration as a simple include
target_compile_definitions(main PRIVATE LV_CONF_INCLUDE_SIMPLE)
target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} m pthread)
target_include_directories(main PRIVATE
    ${UI_DIR}
    ${UI_DIR}/lv_ui
    ${UI_DIR}/lv_ui/pages
    ${UI_DIR}/lv_interface)

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
