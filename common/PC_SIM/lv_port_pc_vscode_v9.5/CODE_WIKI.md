# LVGL PC Simulator (lv_port_pc_vscode) — Code Wiki

> **版本**: LVGL v9.5.0 · FreeRTOS Kernel V11.2.0  
> **项目定位**: 在 PC 上模拟运行 LVGL 嵌入式图形库，支持 FreeRTOS 可选启用，用于嵌入式 GUI 的快速原型开发与调试

---

## 目录

1. [项目概述](#1-项目概述)
2. [项目整体架构](#2-项目整体架构)
3. [目录结构详述](#3-目录结构详述)
4. [主要模块职责](#4-主要模块职责)
5. [关键类/函数说明](#5-关键类函数说明)
6. [依赖关系](#6-依赖关系)
7. [项目构建与运行](#7-项目构建与运行)
8. [配置指南](#8-配置指南)
9. [FreeRTOS 集成](#9-freertos-集成)

---

## 1. 项目概述

### 1.1 项目目标

LVGL 主要面向微控制器和嵌入式系统，但本项目允许在 **PC (Windows/Linux/macOS)** 上直接运行 LVGL 图形应用，无需任何嵌入式硬件。开发完成的代码可直接移植到嵌入式目标板上。

### 1.2 核心技术栈

| 组件 | 技术选型 | 版本 |
|------|----------|------|
| 图形库 | **LVGL** (Light and Versatile Graphics Library) | v9.5.0 |
| 实时内核 | **FreeRTOS** (可选) | Kernel V11.2.0 |
| 窗口/输入 | **SDL2** (Simple DirectMedia Layer) | 系统安装 |
| 构建系统 | **CMake** | ≥ 3.10 |
| 矢量图形 | **ThorVG** (内置于 LVGL) | 内部版本 |
| 开发环境 | **VSCode** + C++ 扩展 | — |

### 1.3 主要特性

- ✅ 跨平台 PC 模拟 (Windows/Linux/macOS)
- ✅ SDL2 窗口渲染 + 硬件加速
- ✅ 鼠标/键盘/滚轮输入模拟
- ✅ FreeRTOS 可选启用（模拟 RTOS 行为）
- ✅ 丰富的 Demo 程序（widgets/benchmark/stress/music 等）
- ✅ 调试配置开箱即用 (GDB/LLDB)
- ✅ 多种外部库支持 (LodePNG/TJPGD/Barcode/QRCode/TinyTTF 等)

---

## 2. 项目整体架构

```
┌─────────────────────────────────────────────────────┐
│                    Application (main)                │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │  Demo 程序    │  │  *_main.cpp  │  │  HAL 初始化 │  │
│  │ (lv_demo_*)   │  │ (FreeRTOS)   │  │ (SDL 窗口) │  │
│  └──────┬───────┘  └──────┬───────┘  └─────┬─────┘  │
│         │                 │                 │         │
└─────────┼─────────────────┼─────────────────┼─────────┘
          │                 │                 │
          ▼                 ▼                 ▼
┌─────────────────────────────────────────────────────┐
│                  LVGL 图形库 (lvgl/)                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐  │
│  │  Core    │ │  Widgets  │ │  Draw    │ │  Misc  │  │
│  │(对象/事件/│ │(按钮/标签/ │ │(渲染引擎) │ │(动画/  │  │
│  │ 刷新/组)  │ │ 图表/...) │ │          │ │ 定时器) │  │
│  └──────────┘ └──────────┘ └──────────┘ └────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐  │
│  │  Drivers │ │  Fonts   │ │  Layouts │ │  Libs  │  │
│  │ (SDL/    │ │(字体渲染) │ │(Flex/    │ │(图片/  │  │
│  │  X11/...) │ │          │ │ Grid)    │ │ 编解码) │  │
│  └──────────┘ └──────────┘ └──────────┘ └────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐              │
│  │  OSAL    │ │  Themes  │ │  Stdlib  │              │
│  │(OS抽象层) │ │(主题系统) │ │(内存管理) │              │
│  └──────────┘ └──────────┘ └──────────┘              │
└──────────────────────┬──────────────────────────────┘
                       │
          ┌────────────┴────────────┐
          ▼                         ▼
┌──────────────────┐   ┌──────────────────────────┐
│  FreeRTOS Kernel  │   │    SDL2 库 (系统安装)      │
│ (可选, 模拟 RTOS) │   │    (窗口/输入/渲染)        │
└──────────────────┘   └──────────────────────────┘
```

### 运行模式

| 模式 | 说明 | 入口 |
|------|------|------|
| **无 RTOS 模式** | 主线程直接轮询 `lv_timer_handler()` | [main.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/main.c) |
| **FreeRTOS 模式** | 创建独立 LVGL 任务，由 FreeRTOS 调度 | [freertos_main.cpp](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/freertos_main.cpp) |

---

## 3. 目录结构详述

```
lv_port_pc_vscode_v9.5/
├── CMakeLists.txt               # 顶层构建配置：SDL2依赖、FreeRTOS开关、编译选项
├── lv_conf.h                    # LVGL 主配置文件 (v9.5.0)
├── lv_conf.defaults             # 配置默认值（用于 Kconfig）
├── simulator.code-workspace     # VSCode 多根工作区配置（含构建/调试/任务）
├── README.md                    # 项目说明文档
├── .gitignore                   # 忽略 build/、bin/、.vscode/ 等
│
├── main/                        # ★ 用户应用层
│   ├── src/
│   │   ├── main.c               # 主入口：初始化 LVGL + HAL，选择运行模式
│   │   ├── freertos_main.cpp    # FreeRTOS 模式：创建任务、启动调度器
│   │   ├── FreeRTOS_Posix_Port.c# 自定义 POSIX 事件工具（event_create/signal/wait）
│   │   └── mouse_cursor_icon.c  # 鼠标光标图标数据（ARGB8888 位图）
│   └── inc/                     # 用户头文件目录（当前为空）
│
├── lvgl/                        # ★ LVGL 图形库 (v9.5.0)
│   ├── src/                     # 核心源码
│   │   ├── core/                # 核心子系统：对象、事件、样式、刷新、组
│   │   ├── display/             # 显示设备管理
│   │   ├── draw/                # 渲染引擎（SW/NXP/SDL/OpenGLES 等）
│   │   ├── drivers/             # 显示/输入设备驱动（SDL/X11/Wayland/evdev 等）
│   │   ├── font/                # 字体引擎与内置字体
│   │   ├── indev/               # 输入设备抽象（鼠标、触摸、键盘、手势）
│   │   ├── layouts/             # 布局引擎（Flex / Grid）
│   │   ├── libs/                # 第三方库集成（LodePNG/TJPGD/QRCode/Barcode/ThorVG/LZ4/SVG 等）
│   │   ├── misc/                # 杂项（动画、定时器、颜色、区域、事件、FS、日志、缓存等）
│   │   ├── osal/                # OS 抽象层（FreeRTOS/Pthread/Windows/SDL2/CMSIS-RTOS2 等）
│   │   ├── stdlib/              # 标准库封装（内存分配器 TLSF、字符串）
│   │   ├── themes/              # 主题系统（Default/Simple/Mono）
│   │   ├── tick/                # 系统滴答
│   │   └── widgets/             # 控件库（Button/Label/Slider/Chart/Table 等 30+）
│   ├── demos/                   # 演示程序（widgets/benchmark/stress/music/multilang 等）
│   ├── examples/                # 示例代码
│   └── scripts/                 # 构建/代码格式化/版本管理脚本
│
├── config/                      # FreeRTOS 配置
│   └── FreeRTOSConfig.h         # FreeRTOS 内核配置（512MB 堆、抢占式调度、5 优先级）
│
├── FreeRTOS/                    # ★ FreeRTOS 内核 (V11.2.0)
│   ├── tasks.c                  # 任务管理核心实现
│   ├── queue.c                  # 队列（消息队列/信号量/互斥锁）
│   ├── timers.c                 # 软件定时器
│   ├── event_groups.c           # 事件组
│   ├── stream_buffer.c          # 流式缓冲区
│   ├── croutine.c               # 协程（默认禁用）
│   ├── list.c                   # 内核链表
│   ├── include/                 # 内核头文件
│   ├── portable/                # 移植层（GCC_POSIX / ARM_CMx / IAR / RVDS 等）
│   │   ├── ThirdParty/GCC/Posix/    # ★ POSIX 移植（PC 模拟用）
│   │   │   ├── port.c               # 每个任务一个 pthread 的 POSIX 移植
│   │   │   ├── portmacro.h          # 移植宏定义
│   │   │   └── utils/wait_for_event.c  # 事件等待机制（pthread_cond）
│   │   └── MemMang/heap_4.c         # 堆内存管理算法（第4方案：合并空闲块）
│   └── CMakeLists.txt           # FreeRTOS 构建配置
│
└── .gitmodules                  # Git 子模块（lv_drivers）
```

---

## 4. 主要模块职责

### 4.1 顶层 CMake ([CMakeLists.txt](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/CMakeLists.txt))

**职责**: 定义整个项目的构建规则。

| 关键配置 | 说明 |
|----------|------|
| `FREERTOS_PORT` | 默认 `GCC_POSIX`，用于 POSIX 环境的 FreeRTOS 移植 |
| `USE_FREERTOS` | 布尔选项，控制是否启用 FreeRTOS（默认 OFF） |
| `LV_USE_DRAW_SDL` | SDL 硬件加速渲染开关 |
| `LV_USE_LIBPNG` / `LV_USE_FFMPEG` / `LV_USE_FREETYPE` | 外部库集成开关 |
| `EXECUTABLE_OUTPUT_PATH` | 可执行文件输出到 `bin/` 目录 |
| `add_custom_target(run)` | `make run` 直接运行程序 |

**可执行文件构建**:
- 无 RTOS: `main.c` + `mouse_cursor_icon.c` → `main`
- 有 RTOS: `main.c` + `freertos_main.cpp` + `FreeRTOS_Posix_Port.c` + FreeRTOS 源码 → `main`

### 4.2 应用入口 ([main/src/](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/))

#### [main.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/main.c)
- **程序入口点** `int main()`
- 流程:
  1. 调用 `lv_init()` 初始化 LVGL
  2. 调用 `hal_init(320, 480)` 初始化 SDL 窗口与输入设备
  3. 根据 `LV_USE_OS` 宏选择运行模式:
     - `LV_OS_NONE`: 直接调用 `lv_demo_widgets()`，然后主循环轮询 `lv_timer_handler()`
     - `LV_OS_FREERTOS`: 委托给 `freertos_main()`

#### [freertos_main.cpp](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/freertos_main.cpp)
- **FreeRTOS 模式入口**: `extern "C" void freertos_main()`
- 创建两个任务:
  - `lvgl_task`: 处理 LVGL 定时器刷新（堆栈 4096 字节）
  - `another_task`: 演示任务，每 500ms 打印消息（堆栈 1024 字节）
- 调用 `vTaskStartScheduler()` 启动调度器
- 提供 FreeRTOS 钩子函数:
  - `vApplicationMallocFailedHook` — malloc 失败处理
  - `vApplicationStackOverflowHook` — 栈溢出检测
  - `vApplicationIdleHook` / `vApplicationTickHook` — 空闲/滴答钩子（空实现）

#### [hal_init()](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/main.c#L107-L134)
- 初始化 SDL 窗口（320×480 像素）
- 创建 3 个输入设备:
  - **鼠标** (`lv_sdl_mouse_create`) + 自定义光标图标
  - **滚轮** (`lv_sdl_mousewheel_create`)
  - **键盘** (`lv_sdl_keyboard_create`)
- 所有输入设备绑定到同一个默认组

#### [mouse_cursor_icon.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/mouse_cursor_icon.c)
- 预定义的鼠标光标 ARGB8888 位图（14×20 像素）

#### [FreeRTOS_Posix_Port.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/FreeRTOS_Posix_Port.c)
- 自定义 POSIX 事件工具（`Event_t` 结构体）
- 提供 `event_create()` / `event_delete()` / `event_signal()` / `event_wait()` 四个函数
- 基于 pthread 条件变量 + 互斥锁实现
- **注意**: 此文件被编译进 main 但未被直接引用，FreeRTOS POSIX 移植使用其自带的 `wait_for_event` 实现

### 4.3 LVGL 图形库 ([lvgl/](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/))

#### 核心子系统 (core/)

| 模块 | 文件 | 职责 |
|------|------|------|
| 对象系统 | [lv_obj.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj.c) | 所有控件的基类：创建/删除/位置/大小/父子关系 |
| 对象类 | [lv_obj_class.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj_class.c) | 基于类的面向对象机制，支持继承 |
| 事件系统 | [lv_obj_event.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj_event.c) | 事件发送/监听/冒泡机制 |
| 样式系统 | [lv_obj_style.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj_style.c) | 样式属性管理（层叠/继承/缓存） |
| 绘制刷新 | [lv_refr.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_refr.c) | 脏区域标记与重绘调度 |
| 对象树 | [lv_obj_tree.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj_tree.c) | 屏幕/层级管理 |
| 观察者 | [lv_observer.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_observer.c) | 观察者模式实现 |
| 组 | [lv_group.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_group.c) | 输入设备焦点组管理 |

#### 渲染引擎 (draw/)

| 模块 | 说明 |
|------|------|
| [lv_draw_sw](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/draw/sw/) | 纯软件渲染器（默认启用），支持复杂绘制（圆角/阴影/弧线/渐变） |
| [lv_draw_sdl](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/draw/sdl/) | SDL 纹理加速渲染（可选，通过 `LV_USE_DRAW_SDL` 开启） |
| [lv_draw_vector](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/draw/lv_draw_vector.c) | 矢量图形 API（需 ThorVG 支持） |
| [lv_draw_image](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/draw/lv_draw_image.c) | 图像解码与绘制 |
| [lv_image_decoder](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/draw/lv_image_decoder.c) | 图像解码器框架 |

#### 控件库 (widgets/)

**30+ 内置控件**，按目录组织:

| 控件目录 | 说明 |
|----------|------|
| `arc/` | 弧形控件 |
| `bar/` | 进度条 |
| `button/` | 按钮 |
| `chart/` | 图表（折线/柱状/散点图） |
| `checkbox/` | 复选框 |
| `dropdown/` | 下拉菜单 |
| `image/` | 图片显示 |
| `label/` | 标签文本 |
| `slider/` | 滑块 |
| `switch/` | 开关 |
| `table/` | 表格 |
| `tabview/` | 标签页 |
| `textarea/` | 文本输入框 |
| `win/` | 窗口容器 |
| ... | 其他 20+ 控件 |

#### 布局引擎 (layouts/)

- **[Flex](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/layouts/flex/)**: CSS Flexbox 风格的弹性布局
- **[Grid](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/layouts/grid/)**: CSS Grid 风格的网格布局

#### 显示驱动 (drivers/)

| 驱动 | 说明 |
|------|------|
| [SDL](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/drivers/sdl/) | PC 窗口 + 输入（本项目默认使用） |
| [X11](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/drivers/x11/) | Linux X11 窗口 |
| [evdev](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/drivers/evdev/) | Linux 输入设备 |
| [libinput](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/drivers/libinput/) | Libinput 输入框架 |
| [Wayland](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/drivers/wayland/) | Wayland 显示协议 |

#### OS 抽象层 (osal/)

LVGL 支持多种 OS 后端，本项目使用 [pthread](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/osal/lv_pthread.c)（默认）或 [FreeRTOS](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/osal/lv_freertos.c)（可选）。

| 后端 | 文件 | 说明 |
|------|------|------|
| Pthread | [lv_pthread.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/osal/lv_pthread.c) | Linux/macOS 多线程 |
| FreeRTOS | [lv_freertos.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/osal/lv_freertos.c) | FreeRTOS 任务/信号量/互斥锁 |
| Windows | [lv_windows.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/osal/lv_windows.c) | Windows 原生线程 |
| None | [lv_os_none.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/osal/lv_os_none.c) | 单线程裸机模式 |

#### 第三方库集成 (libs/)

| 库 | 功能 |
|----|------|
| [LodePNG](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/lodepng/) | PNG 图像解码（默认启用） |
| [TJPGD](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/tjpgd/) | JPEG 图像解码（默认启用） |
| [BMP](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/bmp/) | BMP 图像解码（默认启用） |
| [QRCode](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/qrcode/) | QR 码生成 |
| [Barcode](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/barcode/) | 条形码生成 |
| [ThorVG](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/thorvg/) | 矢量图形渲染引擎（默认启用） |
| [TinyTTF](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/tiny_ttf/) | TTF 字体渲染 |
| [RLE](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/rle/) | RLE 解压缩 |
| [LZ4](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/lz4/) | LZ4 压缩（内部版本） |
| [FSDrv](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/libs/fsdrv/) | 文件系统驱动（stdio/fatfs/win32 等） |

### 4.4 FreeRTOS 内核 ([FreeRTOS/](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/))

| 核心文件 | 职责 |
|----------|------|
| [tasks.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/tasks.c) | 任务创建/删除/调度/阻塞/通知 |
| [queue.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/queue.c) | 队列/二进制信号量/计数信号量/互斥锁 |
| [timers.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/timers.c) | 软件定时器服务 |
| [event_groups.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/event_groups.c) | 事件标志组 |
| [stream_buffer.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/stream_buffer.c) | 流式消息缓冲区 |
| [list.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/list.c) | 内核链表实现 |

#### POSIX 移植 ([portable/ThirdParty/GCC/Posix/](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/portable/ThirdParty/GCC/Posix/))

- **[port.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/portable/ThirdParty/GCC/Posix/port.c)**: 每个 FreeRTOS 任务映射为一个 pthread 线程，使用 SIGALRM 模拟定时器中断，通过 `pthread_cond_wait/signal` 实现任务切换
- **[portmacro.h](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/portable/ThirdParty/GCC/Posix/portmacro.h)**: 数据类型、临界区、栈增长方向等移植宏

---

## 5. 关键类/函数说明

### 5.1 LVGL 核心 API

| 函数 | 位置 | 说明 |
|------|------|------|
| `lv_init()` | [lv_init.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/lv_init.c) | 初始化 LVGL 全局状态、定时器、内存、日志等所有子系统 |
| `lv_timer_handler()` | [lv_timer.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/misc/lv_timer.c) | 处理所有注册的 LVGL 定时器，必须在主循环或任务中周期性调用 |
| `lv_obj_create()` | [lv_obj.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj.c) | 创建控件实例 |
| `lv_label_create()` | [lv_label.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/widgets/label/lv_label.c) | 创建标签控件 |
| `lv_label_set_text()` | 同上 | 设置标签文本 |
| `lv_obj_align()` | [lv_obj_pos.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj_pos.c) | 对齐控件（如 `LV_ALIGN_CENTER`） |
| `lv_scr_load()` | [lv_obj_tree.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/core/lv_obj_tree.c) | 加载并显示指定屏幕 |
| `lv_sdl_window_create()` | [lv_sdl_sw.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/src/drivers/sdl/lv_sdl_sw.c) | 创建 SDL 窗口并返回显示设备句柄 |
| `lv_demo_widgets()` | [lv_demo_widgets.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lvgl/demos) | 启动控件演示程序 |

### 5.2 FreeRTOS 关键函数

| 函数 | 位置 | 说明 |
|------|------|------|
| `xTaskCreate()` | [tasks.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/tasks.c) | 创建新任务 |
| `vTaskStartScheduler()` | 同上 | 启动 FreeRTOS 调度器 |
| `vTaskDelay()` | 同上 | 任务延时 |
| `xPortGetFreeHeapSize()` | [heap_4.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/portable/MemMang/heap_4.c) | 获取剩余堆大小 |

### 5.3 自定义实现

| 函数 | 文件 | 说明 |
|------|------|------|
| `hal_init(w, h)` | [main.c#L107](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/main.c#L107) | 初始化 SDL 窗口 + 鼠标/键盘/滚轮输入 |
| `freertos_main()` | [freertos_main.cpp#L159](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/freertos_main.cpp#L159) | FreeRTOS 模式入口，创建任务并启动调度器 |
| `lvgl_task()` | [freertos_main.cpp#L120](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/freertos_main.cpp#L120) | LVGL 渲染任务，循环调用 `lv_timer_handler()` |
| `create_hello_world_screen()` | [freertos_main.cpp#L82](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/freertos_main.cpp#L82) | 创建简单的 "Hello, World!" 演示屏幕 |
| `event_create/signal/wait/delete()` | [FreeRTOS_Posix_Port.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/main/src/FreeRTOS_Posix_Port.c) | 基于 pthread 的条件变量事件工具 |

---

## 6. 依赖关系

### 6.1 运行时依赖

```
main (可执行文件)
├── lvgl 库 (编译时静态链接)
│   ├── lvgl_thorvg (矢量渲染)
│   ├── lvgl_examples (示例代码)
│   └── lvgl_demos (演示程序)
├── SDL2 库 (动态链接)
│   └── SDL2_image (可选，启用 LV_USE_DRAW_SDL 时)
├── pthread 库 (POSIX 线程)
└── m 库 (数学库)
```

### 6.2 FreeRTOS 可选依赖

```
main (FreeRTOS 模式)
├── freertos_config (FreeRTOSConfig.h 配置接口)
└── FreeRTOS (静态库)
    ├── freertos_kernel_include (内核头文件)
    ├── freertos_kernel_port (POSIX 移植层)
    └── freertos_kernel_port_headers (移植头文件)
```

### 6.3 外部库依赖

| 库 | 用途 | 安装方式 | 本项目默认启用 |
|----|------|----------|:--------:|
| SDL2 | 窗口创建、输入处理、渲染 | 系统包管理器 | **必选** |
| pthread | 多线程支持 | 系统自带 | **必选** |
| SDL2_image | SDL 图像加速 | 可选 | 否 |
| libpng | PNG 解码（替代 LodePNG） | 可选 | 否 |
| libjpeg-turbo | JPEG 解码（替代 TJPGD） | 可选 | 否 |
| FFmpeg | 视频播放 | 可选 | 否 |
| FreeType | 高级字体渲染 | 可选 | 否 |

### 6.4 内部依赖关系（LVGL 模块间）

```
lv_init()
├── misc/lv_timer (定时器系统)
├── misc/lv_anim (动画系统)
├── misc/lv_fs (文件系统)
├── draw/lv_draw (绘制引擎)
├── draw/lv_image_decoder (图像解码)
├── core/lv_obj (对象系统)
├── core/lv_group (输入组)
├── core/lv_refr (刷新引擎)
├── display/lv_display (显示设备)
├── indev/lv_indev (输入设备)
├── layouts/lv_layout (布局引擎)
├── themes (主题系统)
├── libs/* (第三方库初始化)
├── drivers/* (显示驱动初始化)
├── osal/lv_os (OS 抽象层)
└── stdlib/lv_mem (内存分配器)
```

---

## 7. 项目构建与运行

### 7.1 前置条件

| 平台 | 依赖安装命令 |
|------|-------------|
| **Ubuntu/Debian** | `sudo apt-get install -y build-essential libsdl2-dev cmake` |
| **ArchLinux** | `sudo pacman -S sdl2 base-devel gcc make` |
| **macOS** | `brew install sdl2 cmake llvm` |
| **Windows** | 安装 MinGW + SDL2 开发库，或使用 WSL |

### 7.2 VSCode 方式（推荐）

1. 安装 VSCode 扩展: **C/C++**、**CMake Tools**
2. 双击打开 `simulator.code-workspace`
3. 选择 `CMake: [Debug]` 配置
4. 按 `F5` 或点击 "Run and Debug" → 选择 `Debug LVGL demo with gdb`
5. 程序自动构建并运行，显示 SDL 窗口

### 7.3 CMake 命令行方式

```bash
# 进入项目根目录
cd lv_port_pc_vscode_v9.5

# 创建构建目录
mkdir build && cd build

# 配置（默认无 FreeRTOS）
cmake ..

# 构建
make -j$(nproc)

# 运行
./bin/main
```

### 7.4 启用 FreeRTOS

```bash
# 配置时启用 FreeRTOS
cmake .. -DUSE_FREERTOS=ON

# 同时需要在 lv_conf.h 中设置
# #define LV_USE_OS LV_OS_FREERTOS

# 构建
make -j$(nproc)
```

### 7.5 构建选项

| CMake 选项 | 默认值 | 说明 |
|------------|--------|------|
| `USE_FREERTOS` | `OFF` | 启用 FreeRTOS 内核 |
| `LV_USE_DRAW_SDL` | `OFF` | SDL 硬件加速渲染 |
| `LV_USE_LIBPNG` | `OFF` | 使用 libpng 解码 PNG |
| `LV_USE_LIBJPEG_TURBO` | `OFF` | 使用 libjpeg-turbo 解码 JPEG |
| `LV_USE_FFMPEG` | `OFF` | 启用 FFmpeg 视频播放 |
| `LV_USE_FREETYPE` | `OFF` | 启用 FreeType 字体渲染 |
| `ASAN` | `OFF` | 启用 AddressSanitizer（Debug 模式） |

---

## 8. 配置指南

### 8.1 LVGL 配置 ([lv_conf.h](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/lv_conf.h))

该文件是 LVGL 的核心配置，包含数百个可调参数。关键配置项:

| 配置 | 值 | 说明 |
|------|:--:|------|
| `LV_COLOR_DEPTH` | 32 | 32位 XRGB8888 颜色深度 |
| `LV_MEM_SIZE` | 1MB | LVGL 内部内存池大小 |
| `LV_USE_OS` | `LV_OS_NONE` | 操作系统后端（默认无OS，可改为 `LV_OS_FREERTOS` 或 `LV_OS_PTHREAD`） |
| `LV_USE_SDL` | 1 | 启用 SDL 显示驱动 |
| `LV_USE_LOG` | 1 | 启用日志输出（级别 `LV_LOG_LEVEL_WARN`） |
| `LV_USE_THORVG_INTERNAL` | 1 | 启用内置 ThorVG 矢量渲染 |
| `LV_USE_VECTOR_GRAPHIC` | 1 | 启用矢量图形 API |
| `LV_USE_FLOAT` | 1 | 启用浮点运算支持 |
| `LV_USE_MATRIX` | 1 | 启用矩阵变换支持 |
| `LV_BUILD_EXAMPLES` | 1 | 构建示例代码 |
| `LV_BUILD_DEMOS` | 1 | 构建演示程序 |

### 8.2 FreeRTOS 配置 ([FreeRTOSConfig.h](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/config/FreeRTOSConfig.h))

| 配置 | 值 | 说明 |
|------|:--:|------|
| `configUSE_PREEMPTION` | 1 | 抢占式调度 |
| `configTICK_RATE_HZ` | 1000 | 系统滴答频率 1kHz |
| `configMAX_PRIORITIES` | 5 | 最大优先级数 |
| `configTOTAL_HEAP_SIZE` | 512MB | FreeRTOS 堆大小（PC 模拟需要大堆） |
| `configMINIMAL_STACK_SIZE` | 256 | 最小任务栈大小 |
| `configUSE_TIMERS` | 1 | 启用软件定时器 |
| `configUSE_MUTEXES` | 1 | 启用互斥锁 |
| `configCHECK_FOR_STACK_OVERFLOW` | 0 | 栈溢出检测（关闭） |

---

## 9. FreeRTOS 集成

### 9.1 启用方式

两步操作:

1. **CMake 开关**: `cmake -DUSE_FREERTOS=ON ..`
2. **LVGL 配置**: 在 `lv_conf.h` 中将 `LV_USE_OS` 改为 `LV_OS_FREERTOS`

### 9.2 运行流程

```
main()
  ├─ lv_init()             ← LVGL 初始化
  ├─ hal_init(320, 480)    ← SDL 窗口 + 输入设备
  └─ freertos_main()       ← FreeRTOS 入口
       ├─ xTaskCreate(lvgl_task, ...)    ← 创建 LVGL 渲染任务
       ├─ xTaskCreate(another_task, ...)  ← 创建演示任务
       └─ vTaskStartScheduler()          ← 启动调度器（不再返回）
```

### 9.3 POSIX 移植原理

FreeRTOS 的 POSIX 移植 ([port.c](file:///c:/Users/Administrator/Desktop/lv_port_pc_vscode_v9.5/FreeRTOS/portable/ThirdParty/GCC/Posix/port.c)) 采用 **1 个 FreeRTOS 任务 = 1 个 pthread 线程** 的映射方式:

- 每个任务有独立的 pthread 线程
- 非运行态的任务通过 `pthread_cond_wait` 阻塞等待
- 任务切换通过 `pthread_cond_signal` 唤醒目标线程实现
- 定时器中断通过 `SIGALRM` 信号实现
- 空闲任务通过 `wait_for_event` 机制等待

### 9.4 重要注意事项

- FreeRTOS 堆大小配置为 **512MB**，这是 PC 模拟稳定运行的关键参数
- 使用 FreeRTOS 时，LVGL 的 `lv_timer_handler()` 由 FreeRTOS 任务调用，而非主循环
- stdio 函数（printf 等）在多任务中应使用 FreeRTOS 互斥锁保护
- macOS 调试时需在 `~/.lldbinit` 中添加 `process handle SIGUSR1 -n true -p false -s false` 以抑制信号干扰

---

> **文档生成日期**: 2026-08-14  
> **项目源码**: [lv_port_pc_vscode](https://github.com/lvgl/lv_port_pc_vscode)  
> **LVGL 官方文档**: https://docs.lvgl.io/