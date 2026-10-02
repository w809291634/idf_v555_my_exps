# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是 **LVGL v9.5** 的 PC 模拟器项目（基于 `lvgl/lv_port_pc_vscode`），用于在没有嵌入式硬件的情况下在 PC (Windows/Linux/macOS) 上运行 LVGL 图形应用。代码可直接移植到嵌入式目标板。

- 图形库：LVGL v9.5.0
- 窗口/输入：SDL2
- 实时内核：FreeRTOS Kernel V11.2.0（可选）
- 构建系统：CMake (≥3.10)
- IDE：VSCode（推荐）

更详细的架构说明见 [CODE_WIKI.md](CODE_WIKI.md)。

---

## 常用命令

### 前置依赖（Ubuntu）

```bash
sudo apt-get update && sudo apt-get install -y build-essential libsdl2-dev cmake
```

macOS 需要 `brew install sdl2 cmake llvm`（macOS 自带的 clang 不支持 `-fsanitize=leak`，如需 ASAN 必须用 Homebrew 的 llvm）。

Windows 推荐使用 WSL 或安装 MinGW + SDL2 开发库（gdb 路径：`C:\MinGw\bin\gdb.exe`，已在 `simulator.code-workspace` 中配置）。

### VSCode 方式（推荐）

1. 用 VSCode 打开 `simulator.code-workspace`
2. 安装推荐扩展：`ms-vscode.cpptools`、`ms-vscode.cmake-tools`
3. 选择 `CMake: [Debug]` kit（默认已 `configureOnOpen: true`）
4. `F5` 启动调试（launch 配置：`Debug LVGL demo with gdb` 或 `Debug LVGL demo with LLVM`）

workspace 已配置 `Build`、`Build and Run` 任务。

### 命令行方式

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行
./bin/main         # 或 cmake --build .. --target run
```

可执行文件输出到 `${PROJECT_SOURCE_DIR}/bin/main`。

### CMake 构建选项

| 选项 | 默认 | 说明 |
|------|:----:|------|
| `USE_FREERTOS` | OFF | 启用 FreeRTOS 内核 |
| `LV_USE_DRAW_SDL` | OFF | SDL 硬件加速渲染（需 SDL2_image） |
| `LV_USE_LIBPNG` | OFF | 使用 libpng 替代 LodePNG |
| `LV_USE_LIBJPEG_TURBO` | OFF | 使用 libjpeg-turbo 替代 TJPGD |
| `LV_USE_FFMPEG` | OFF | 启用 FFmpeg 视频播放 |
| `LV_USE_FREETYPE` | OFF | 启用 FreeType 字体渲染 |
| `ASAN` | OFF | 启用 AddressSanitizer（仅 Debug 模式生效） |

启用 FreeRTOS 时**必须同时**设置：
```bash
cmake .. -DUSE_FREERTOS=ON
```
并在 `lv_conf.h` 中把 `#define LV_USE_OS   LV_OS_NONE` 改为 `#define LV_USE_OS   LV_OS_FREERTOS`。

---

## 项目结构

```
lv_port_pc_vscode_v9.5/
├── CMakeLists.txt                  # 顶层构建配置
├── lv_conf.h / lv_conf.defaults    # LVGL 配置
├── simulator.code-workspace        # VSCode 多根工作区
├── main/                           # ★ 用户应用层（少量代码，改这里）
│   └── src/
│       ├── main.c                  # 程序入口；HAL 初始化；选择运行模式
│       ├── freertos_main.cpp       # FreeRTOS 模式入口（仅 USE_FREERTOS=ON 编译）
│       ├── FreeRTOS_Posix_Port.c   # 自定义 POSIX 事件工具（编译进 main 但未被使用）
│       └── mouse_cursor_icon.c     # 鼠标光标 ARGB8888 位图
├── lvgl/                           # LVGL v9.5 源码（核心/控件/驱动/字体/布局...）
├── FreeRTOS/                       # FreeRTOS Kernel V11.2.0（POSIX 移植）
└── config/
    └── FreeRTOSConfig.h            # FreeRTOS 内核配置（关键：512MB 堆）
```

LVGL 内部按子系统组织：`core/` `display/` `draw/` `drivers/` `widgets/` `layouts/` `font/` `libs/` `misc/` `osal/` `stdlib/` `themes/` `tick/` `indev/`。

---

## 架构与代码入口

### 应用入口（用户修改重点）

- **`main/src/main.c`**：`int main()`
  1. `lv_init()` 初始化 LVGL 全局状态
  2. `hal_init(320, 480)` 初始化 SDL 窗口与 3 个输入设备
  3. 根据 `LV_USE_OS` 宏分支：
     - `LV_OS_NONE` → 调用 `lv_demo_widgets()`，主循环 `lv_timer_handler()` + `usleep(5ms)`
     - `LV_OS_FREERTOS` → 调用 `freertos_main()`（不再返回）

- **`hal_init()`** (`main/src/main.c` L107-134)
  - 创建默认组 `lv_group_set_default(lv_group_create())`
  - `lv_sdl_window_create(w, h)` 创建显示
  - `lv_sdl_mouse_create` + `mouse_cursor_icon`（自定义光标）
  - `lv_sdl_mousewheel_create`、`lv_sdl_keyboard_create`
  - 三个输入设备都绑定到同一默认组

- **`main/src/freertos_main.cpp`**：`extern "C" void freertos_main()`
  - 创建两个任务：`lvgl_task`（堆栈 4096）和 `another_task`（堆栈 1024）
  - `lvgl_task` 创建 Hello World 屏幕 + 循环 `lv_timer_handler()` + `vTaskDelay(pdMS_TO_TICKS(5))`
  - 实现 4 个 FreeRTOS 钩子：`vApplicationMallocFailedHook` / `vApplicationStackOverflowHook` / `vApplicationIdleHook` / `vApplicationTickHook`
  - 整个文件用 `#if LV_USE_OS == LV_OS_FREERTOS` 包裹，宏不开启时不编译

- **`main/src/FreeRTOS_Posix_Port.c`**：基于 pthread 条件变量的事件工具。当前**编译进 main 但未被引用**——FreeRTOS POSIX 移植自带 `wait_for_event` 实现。如要使用可考虑移除此文件避免混淆。

### LVGL 子系统依赖（`lv_init()` 时初始化顺序）

`timer` → `anim` → `fs` → `draw` → `image_decoder` → `obj` → `group` → `refr` → `display` → `indev` → `layout` → `themes` → `libs/*` → `drivers/*` → `os` → `mem`

### LVGL → FreeRTOS 集成

- LVGL 通过 `lvgl/src/osal/lv_freertos.c`（当 `LV_USE_OS == LV_OS_FREERTOS` 时编译）调用 FreeRTOS 任务/信号量/互斥锁
- FreeRTOS POSIX 移植在 `FreeRTOS/portable/ThirdParty/GCC/Posix/port.c`，每个 FreeRTOS 任务 = 1 个 pthread 线程，通过 `SIGALRM` 模拟 tick、`pthread_cond_wait/signal` 实现切换

---

## 关键配置

### `lv_conf.h` 关键项（LVGL v9.5）

| 宏 | 当前值 | 含义 |
|----|:------:|------|
| `LV_COLOR_DEPTH` | 32 | XRGB8888 |
| `LV_MEM_SIZE` | 1MB | LVGL 内存池 |
| `LV_USE_OS` | `LV_OS_NONE` | OS 后端（FreeRTOS 模式要改 `LV_OS_FREERTOS`） |
| `LV_USE_SDL` | 1 | SDL 显示驱动 |
| `LV_USE_LOG` | 1 | 日志（级别 `LV_LOG_LEVEL_WARN`） |
| `LV_USE_THORVG_INTERNAL` | 1 | 内置 ThorVG 矢量渲染 |
| `LV_USE_VECTOR_GRAPHIC` | 1 | 矢量图形 API |
| `LV_USE_FLOAT` / `LV_USE_MATRIX` | 1 / 1 | 浮点 / 矩阵变换 |
| `LV_BUILD_EXAMPLES` / `LV_BUILD_DEMOS` | 1 / 1 | 构建示例 / 演示 |

修改此文件后**必须重新运行 cmake 配置**（或重新 generate）才生效。

### `config/FreeRTOSConfig.h` 关键项

| 宏 | 值 | 含义 |
|----|:--:|------|
| `configTICK_RATE_HZ` | 1000 | 1kHz 系统滴答 |
| `configMAX_PRIORITIES` | 5 | 最大优先级数 |
| `configTOTAL_HEAP_SIZE` | **512MB** | **PC 模拟必须保持够大**，否则 SDL 窗口延迟甚至无法显示 |
| `configMINIMAL_STACK_SIZE` | 256 | 最小任务栈 |
| `configUSE_PREEMPTION` | 1 | 抢占式调度 |
| `configCHECK_FOR_STACK_OVERFLOW` | 0 | 栈溢出检测（关闭） |

> ⚠️ FreeRTOS 堆是 PC 模拟能稳定运行的关键参数，README 中特别强调减小此值会导致 SDL 窗口延迟或不显示。

---

## 调试技巧

- **ASAN**：在 Debug 模式下加 `-DASAN=ON` 配置，编译时会自动加 `-fsanitize=address`。
- **macOS**：在 `~/.lldbinit` 加 `process handle SIGUSR1 -n true -p false -s false` 抑制 FreeRTOS POSIX 移植产生的 SIGUSR1。
- **VSCode 调试器**：Linux 用 gdb，macOS 用 lldb，Windows gdb 路径在 workspace 中已设为 `C:\MinGw\bin\gdb.exe`。
- **性能监视器**：`lv_conf.h` 中 `LV_USE_PERF_MONITOR` 控制是否显示 FPS/CPU。

---

## 演示程序（构建默认启用）

通过 `lv_conf.h` / `lv_conf.defaults` ��� `LV_USE_DEMO_*` 开关：

- `widgets` — 控件展示（默认 `main.c` 调用 `lv_demo_widgets()`）
- `benchmark` — 渲染基准测试
- `stress` — 压力测试
- `music` — 音乐播放器 UI
- `flex_layout` / `transform` / `scroll` / `multilang` / `keypad_and_encoder` / `render`

切换演示只需改 `main/src/main.c` 中第 80 行 `lv_demo_widgets()` 调用。

---

## 依赖说明

**必选（运行时）**：SDL2、pthread、m（数学库）

**可选（外部库）**：SDL2_image、libpng、libjpeg-turbo、FFmpeg、FreeType — 在 CMake 中通过 `-D` 选项启用，启用后会被 `find_package` 找到并链接。

**LVGL 内置（默认启用）**：LodePNG、TJPGD、BMP、QRCode、Barcode、ThorVG、TinyTTF、RLE、LZ4、FSDrv。