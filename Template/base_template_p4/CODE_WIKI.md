# base_template_p4 — Code Wiki

> 基于 **ESP-IDF v5.5.5** 的 **ESP32-P4** 基础工程模板（另有 `base_template_c61` 姊妹模板）。
> 该模板以"开箱即用的最小骨架"为目标：内置 **交互式控制台（esp_console，仅基础 system 命令）**、**CPU 使用率监控**、**LED 驱动** 与 **板级配置抽象**，可在此之上快速搭建新项目。

---

## 1. 项目定位与整体架构

### 1.1 是什么

- 一个 **可复用的工程骨架**（Template），不是完整业务应用。
- 启动后打印芯片/Flash/堆信息，初始化 LED，启动控制台任务与 CPU 使用率统计定时器，然后主循环空转。
- 所有硬件引脚、任务参数、开关均通过 **宏配置**（`board_config.h` / `Kconfig`）控制，做到"改配置不改代码"。

### 1.2 分层架构

项目采用 **分层组件** 设计，典型的三层结构：

| 层次 | 组件 | 职责 |
| ---- | ---- | ---- |
| 应用层 | `main` | 程序入口 `app_main()`，应用初始化 `app_init()` |
| 板级层 | `board` | 硬件板级初始化、CPU 使用率监控 |
| 配置层 | `board_config` | 引脚/任务宏定义、调试宏（`mydbg.h`） |
| 应用组件（共享） | `apl_console`、`apl_utility`、`apl_console_cmd_*` | 控制台、工具函数、控制台命令 |
| 驱动组件（共享） | `drv_led` | LED GPIO 驱动 |

> `APL`（Application Layer）/ `DRV`（Driver Layer）共享组件不在本模板目录内，而是统一存放在
> `$IDF_PATH/examples/idf_v555_my_exps/common/{APL,DRV}`，通过根 `CMakeLists.txt` 动态扫描加入编译。

### 1.3 启动流程

```
上电 → app_main()
        ├─ hw_board_init()            [board 组件]
        │     ├─ apl_console_init()   (CONFIG_APP_ENABLE_CONSOLE=y 时) 创建控制台任务
        │     └─ setupCpuUsageMonitor() 创建 1s 周期软件定时器统计各任务 CPU 占用
        ├─ app_init()
        │     ├─ app_info_dump()      打印芯片/Flash/堆信息
        │     └─ led_pin_init()       初始化 LED GPIO
        └─ for(;;) vTaskDelay(1000)   主循环空转
```

---

## 2. 目录结构

```
base_template_p4/
├── CMakeLists.txt                  # 工程根构建脚本（组件扫描 + 共享组件引入）
├── README.md                       # 官方说明（控制台历史/转义字符注意事项）
├── sdkconfig.defaults              # 默认配置（注意：内容实际为 esp32c61 配置，见 §7 风险项）
├── sdkconfig.defaults.esp32c61     # esp32c61 补充配置（PSRAM）
├── partitions_example.csv          # 分区表（nvs / phy_init / factory）
├── .gitkeep
│
├── main/                           # 应用层
│   ├── CMakeLists.txt
│   ├── Kconfig.projbuild           # 引入共享组件的 Kconfig（orsource）
│   ├── include/.gitkeep
│   └── src/
│       └── main.c                  # app_main / app_init
│
└── components/                     # 板级层
    ├── board/                      # 板级初始化 + CPU 监控
    │   ├── board.c
    │   └── CMakeLists.txt
    └── board_config/               # 板级配置 + 调试宏
        ├── board.h                 # 板级 API 声明（含 FreeRTOS 扩展函数）
        ├── board_config.h          # 引脚 / 任务宏配置
        ├── mydbg.h                 # 类 RT-Thread 风格日志宏
        └── CMakeLists.txt
```

**外部共享依赖**（位于 `$IDF_PATH/examples/idf_v555_my_exps/common/`）：

```
common/
├── APL/
│   ├── apl_console/                # 控制台核心（任务 + 初始化）
│   ├── apl_console_cmd_system/     # system 命令组（version/reboot/free/heap/mem/tasks/ps/log_level/睡眠）
│   ├── apl_console_cmd_nvs/        # nvs 命令组（set/get/erase/namespace/list）
│   ├── apl_console_cmd_wifi/       # wifi 命令组（join）
│   ├── apl_utility/                # app_info_dump / print_binary
│   └── apl_atomic/                 # 原子操作封装（本模板未直接使用）
└── DRV/
    ├── drv_led/                    # LED 驱动
    ├── drv_key/                    # 按键驱动（本模板未直接使用）
    └── drv_input/                  # 输入抽象（本模板未直接使用）
```

---

## 3. 模块职责与关键函数

### 3.1 `main`（应用层）

文件：[main.c](file:///d:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/Template/base_template_p4/main/src/main.c)

| 函数 | 说明 |
| ---- | ---- |
| `app_main()` | FreeRTOS 入口。调 `hw_board_init()` → `app_init()` → 空转循环 |
| `app_init()` | 应用初始化：`app_info_dump()` 打印系统信息、`led_pin_init()` 初始化 LED |

编译配置见 [main/CMakeLists.txt](file:///d:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/Template/base_template_p4/main/CMakeLists.txt)：
- `REQUIRES spi_flash drv_led apl_utility`
- `PRIV_REQUIRES board board_config`

### 3.2 `board`（板级层）

文件：[board.c](file:///d:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/Template/base_template_p4/components/board/board.c)

| 函数 | 说明 |
| ---- | ---- |
| `hw_board_init()` | 板级初始化。`CONFIG_APP_ENABLE_CONSOLE` 时启动控制台；`CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS` 时启动 CPU 监控 |
| `setupCpuUsageMonitor()` | 创建 1s 周期、自动重载的 FreeRTOS 软件定时器 `CpuUsageTimer` |
| `vTimerCallback()` | 定时器回调：通过 `uxTaskGetSystemState()` 枚举全部任务，计算每个任务 CPU 占用百分比并写入 TCB（`vTaskSetCpuUsagePercent`），随后重置运行计数器 |

> CPU 使用率计算方式：`百分比 = 任务运行计数 / 定时周期内总运行计数 × 100%`。
> `last_uxTotalRunTime` 为静态变量，用于计算相邻两次采样的增量。

依赖：`REQUIRES apl_utility apl_console board_config`。

### 3.3 `board_config`（配置层）

| 文件 | 内容 |
| ---- | ---- |
| [board_config.h](file:///d:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/Template/base_template_p4/components/board_config/board_config.h) | 引脚宏（LED/KEY，当前全部注释掉）与任务宏：`BOARD_CONFIG_CONSOLE_TASK_STACK_SIZE=4096`、`BOARD_CONFIG_CONSOLE_TASK_PRIOR=10`、`BOARD_CONFIG_CONSOLE_TASK_CPU=0` |
| [board.h](file:///d:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/Template/base_template_p4/components/board_config/board.h) | 板级 API：`hw_board_init()` + 4 个 FreeRTOS 任务扩展函数声明 |
| [mydbg.h](file:///d:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/Template/base_template_p4/components/board_config/mydbg.h) | 类 RT-Thread 日志宏：`log_d/log_i/log_w/log_e`、`logf_*`、`logfn_*`、`log_raw`、`_ASSERT` 等，支持分级（`LEVEL_TYPE 0`：按等级阈值；`LEVEL_TYPE 1`：按位控制）与 ANSI 颜色 |

**mydbg 使用方式**：源文件顶部先定义 `DBG_TAG` 与 `DBG_LVL`，再 `#include <mydbg.h>`。

**board.h 中的 FreeRTOS 扩展函数**（实现位于 IDF 内核，见 §5）：

| 函数 | 说明 |
| ---- | ---- |
| `vTaskGetStackSize(TaskHandle_t)` | 获取任务栈总大小（直接读 TCB `uxSizeOfStack`） |
| `vTaskResetRunTimeCounter(TaskHandle_t)` | 清零任务运行时间计数 |
| `vTaskSetCpuUsagePercent(TaskHandle_t, float)` | 将 CPU 占用百分比写入 TCB 字段 |
| `vTaskGetCpuUsagePercent(TaskHandle_t)` | 读取任务 CPU 占用百分比 |

### 3.4 `apl_console`（共享控制台）

文件：`common/APL/apl_console/apl_console.c`、`console_settings.c`

| 函数 | 说明 |
| ---- | ---- |
| `apl_console_init()` | 初始化文件系统（`CONSOLE_STORE_HISTORY` 时挂载 FAT）、初始化外设与控制台库，注册命令，创建 `apl_console` 任务 |
| `apl_console_task()` | 控制台主循环：`linenoise()` 读取输入 → 加入历史 → `esp_console_run()` 执行 |
| `initialize_console_peripheral()` | 依据 `CONFIG_ESP_CONSOLE_*` 选择 UART / USB_CDC / USB_SERIAL_JTAG 作为控制台通道 |
| `initialize_console_library()` | 初始化 `esp_console` 与 `linenoise`，探测终端转义序列支持 |
| `setup_prompt()` | 构造带颜色前缀的命令提示符（默认 `CONFIG_IDF_TARGET>`） |

- 命令注册条件（可在 `board_config.h` 中用 `BOARD_CONFIG_ENABLE_*_CMD` 开关关闭）：
  `system` / `sleep`（`SOC_*_SLEEP_SUPPORTED`）/ `wifi`（WiFi 使能时）/ `nvs`。
- 任务创建使用 `xTaskCreatePinnedToCoreWithCaps`，栈可放 PSRAM（`BOARD_CONFIG_CONSOLE_TASK_STACK_IN_PSRAM`）。

依赖：`REQUIRES apl_console_cmd_nvs apl_console_cmd_system apl_console_cmd_wifi fatfs board_config`；`PRIV_REQUIRES console esp_driver_uart fatfs esp_driver_usb_serial_jtag nvs_flash`。

### 3.5 控制台命令组

| 组件 | 注册函数 | 可用命令 |
| ---- | ---- | ---- |
| `apl_console_cmd_system` | `register_system_common()` / `register_system_light_sleep()` / `register_system_deep_sleep()` | `version`、`reboot`、`free`、`heap`、`mem`、`tasks`、`ps`、`log_level`、`light_sleep`、`deep_sleep` |
| `apl_console_cmd_nvs` | `register_nvs()` | `nvs set/get/erase/namespace/list` |
| `apl_console_cmd_wifi` | `register_wifi()` | `join`（连接 AP） |

> 本 P4 基础工程中仅启用 `system` 基础命令组（`version`/`reboot`/`free`/`heap`/`mem`/`tasks`/`ps`/`log_level`），
> `sleep` / `wifi` / `nvs` 命令通过 `board_config.h` 中的 `BOARD_CONFIG_ENABLE_*_CMD` 宏关闭（见 §6.1）。

### 3.6 `apl_utility`（共享工具）

文件：`common/APL/apl_utility/apl_utility.c`

| 函数 | 说明 |
| ---- | ---- |
| `app_info_dump()` | 打印芯片型号/核心数/特性（WiFi/BT/BLE/802.15.4）、硅片修订版本、Flash 大小与类型、最小空闲堆 |
| `print_binary(uint32_t val, int bits)` | 以二进制打印 32 位数值，每 4 位加空格 |

### 3.7 `drv_led`（共享驱动）

文件：`common/DRV/drv_led/src/drv_led.c`

| 函数 | 说明 |
| ---- | ---- |
| `led_pin_init()` | 按 `BOARD_CONFIG_HAL_LEDx_GPIO` 宏初始化 0~3 号 LED 引脚为输出 |
| `led_ctrl(unsigned char cmd)` | 位图控制：每 bit 对应一个 LED，`BOARD_CONFIG_HAL_LEDx_ACTIVATE` 决定高/低电平点亮 |
| `led_write(index, value)` | 单独控制某个 LED（`value` 1=亮 0=灭） |

> 当前模板中 `BOARD_CONFIG_HAL_LED*_GPIO` 宏均被注释，LED 实际引脚需按目标板在 `board_config.h` 中开启。

---

## 4. 依赖关系

### 4.1 组件依赖图

```
                        ┌──────────────┐
                        │     main     │  REQUIRES: spi_flash, drv_led, apl_utility
                        │              │  PRIV_REQUIRES: board, board_config
                        └──────┬───────┘
                               │
        ┌──────────────────────┼───────────────────────┐
        │                      │                       │
   ┌────▼─────┐          ┌─────▼─────┐           ┌─────▼──────┐
   │  board   │          │ board_config (仅头文件/宏) │
   │ REQUIRES │          └───────────┘
   │ apl_utility, apl_console, board_config
   └────┬─────┘
        │
   ┌────▼──────────────┐
   │    apl_console     │ REQUIRES: apl_console_cmd_{system,nvs,wifi}, fatfs, board_config
   │ PRIV_REQUIRES: console, esp_driver_uart, esp_driver_usb_serial_jtag, nvs_flash, fatfs
   └────┬──────────────┘
        │
   ┌────▼─────────────────────────┐
   │ apl_console_cmd_system       │ REQUIRES: console, spi_flash, esp_driver_uart, esp_driver_gpio, board_config
   │ apl_console_cmd_nvs          │ REQUIRES: console, nvs_flash
   │ apl_console_cmd_wifi         │ REQUIRES: console, esp_wifi
   └──────────────────────────────┘

   drv_led / apl_utility          REQUIRES: esp_driver_gpio / spi_flash + board_config
```

### 4.2 外部路径依赖

根 [CMakeLists.txt](file:///d:/esp32_8266_files/esp-idf-v5.5.5_ol/examples/idf_v555_my_exps/Template/base_template_p4/CMakeLists.txt) 通过自定义函数 `add_component_dirs_from()` 将共享组件目录逐个加入 `EXTRA_COMPONENT_DIRS`：

```cmake
set(APL_DIR "$ENV{IDF_PATH}/examples/idf_v555_my_exps/common/APL")
set(DRV_DIR "$ENV{IDF_PATH}/examples/idf_v555_my_exps/common/DRV")
```

同时通过 `set(COMPONENTS main esp_psram)` 裁剪组件以缩短编译时间（依赖组件仍会被自动引入）。

---

## 5. FreeRTOS 内核定制扩展（本仓库关键前提）

本项目的 CPU 使用率监控依赖 **本地 IDF 中已修改的 FreeRTOS 内核**（非官方上游行为），包括：

1. **TCB 新增字段**：`float cpuUsagePercent;`（位于 `configGENERATE_RUN_TIME_STATS == 1` 条件下）
   - 文件：`components/freertos/FreeRTOS-Kernel/tasks.c`（约 L461-L464）
2. **新增 API**（`components/freertos/FreeRTOS-Kernel/tasks.c` 末尾 "User Function" 区，约 L6491-L6512）：
   - `uint32_t vTaskGetStackSize(TaskHandle_t)` — 返回 `uxSizeOfStack`
   - `void vTaskResetRunTimeCounter(TaskHandle_t)` — 清零 `ulRunTimeCounter`
   - `float vTaskGetCpuUsagePercent(TaskHandle_t)` / `void vTaskSetCpuUsagePercent(...)`
3. 这些 API 由 `board_config/board.h` 声明、`board` 组件写入、`apl_console_cmd_system` 的 `tasks/ps` 命令读取显示。

> 移植到其他 IDF 环境时，必须同步迁移这组内核补丁，否则 `tasks/ps` 命令与 CPU 监控将链接失败。

---

## 6. 配置项总览

### 6.1 Kconfig（`main/Kconfig.projbuild`）

| 配置 | 默认 | 说明 |
| ---- | ---- | ---- |
| `APP_ENABLE_CONSOLE` | y | 是否启用控制台 |
| `CONSOLE_STORE_HISTORY` | n | 是否将命令历史存入 Flash（需 FAT 分区） |
| `CONSOLE_IGNORE_EMPTY_LINES` | y | 忽略空行（否则 EOF 即退出控制台） |

> `Kconfig.projbuild` 使用 `orsource` 引入共享组件的 Kconfig，缺失时不会报错（可选）。

### 6.2 sdkconfig.defaults（关键项）

| 配置 | 值 | 说明 |
| ---- | ---- | ---- |
| `CONFIG_IDF_TARGET` | "esp32p4" | 目标芯片 ESP32-P4 |
| `CONFIG_ESP32P4_REV_MIN_301` | y | 最低硅片版本 3.0.1 |
| `CONFIG_ESPTOOLPY_FLASHSIZE_16MB` | y | 16MB Flash |
| `CONFIG_ESPTOOLPY_FLASHMODE_QIO` | y | QIO 模式 |
| `CONFIG_PARTITION_TABLE_CUSTOM` | y | 使用 `partitions_example.csv` |
| `CONFIG_ESP_MAIN_TASK_STACK_SIZE` | 7168 | 主任务栈 |
| `CONFIG_FREERTOS_USE_TRACE_FACILITY` / `USE_STATS_FORMATTING_FUNCTIONS` | y | `tasks/ps` 命令依赖 |
| `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS` | y | CPU 使用率统计依赖 |
| `CONFIG_ESP_CONSOLE_SECONDARY_NONE` | y | 关闭 USB 第二控制台 |
| `CONFIG_ESP_CONSOLE_UART_CUSTOM` | y | 控制台使用自定义 UART（GPIO37/38，961200 baud，参考 p4_touch 板卡） |

> 控制台命令开关（`board_config.h`）：`BOARD_CONFIG_ENABLE_SYSTEM_CMD=1`（保留基础命令），
> `BOARD_CONFIG_ENABLE_{SLEEP,WIFI,NVS}_CMD=0`（默认关闭，按需置 1）。

`sdkconfig.defaults.esp32p4`：PSRAM（`CONFIG_SPIRAM=y`、200MHz、`SPIRAM_XIP_FROM_PSRAM`）。

### 6.3 分区表（partitions_example.csv）

> ⚠️ ESP32-P4 的 bootloader 较大（约 0x60C0，超过默认 0x6000 上限），
> 分区表偏移必须设置为 `0x10000`（`CONFIG_PARTITION_TABLE_OFFSET=0x10000`），否则构建报
> "Bootloader binary size too large" 错误。

| 分区 | 类型/子类型 | 偏移 | 大小 |
| ---- | ---- | ---- | ---- |
| `nvs` | data/nvs | 0x11000 | 0x6000 |
| `phy_init` | data/phy | 自动（紧跟 nvs） | 0x1000 |
| `factory` | app/factory | 自动 | 2M |

> ⚠️ `README.md` 提示：若启用 `CONSOLE_STORE_HISTORY`，需要额外添加 `storage, data, fat, , 1M` 分区，
> 但当前 `partitions_example.csv` **并未包含**该分区。启用历史存储前需自行补充分区。

---

## 7. 已知问题 / 风险项（排查前先看这里）

| # | 事项 | 说明/处理建议 |
| ---- | ---- | ---- |
| 1 | 外部共享组件路径 | 已修正为 `idf_v555_my_exps`；若目录改名需同步修改根 `CMakeLists.txt` 与 `main/Kconfig.projbuild` |
| 2 | 目标芯片 | 已改为 **esp32p4**（含 `CONFIG_ESP32P4_REV_MIN_301`）；首次构建自动生成 `sdkconfig` |
| 3 | 控制台命令裁剪 | 仅保留 system 基础命令；`sleep/wifi/nvs` 通过 `board_config.h` 的 `BOARD_CONFIG_ENABLE_*_CMD` 关闭，需要时改为 1 |
| 4 | `BOARD_CONFIG_CONSOLE_TASK_STACK_IN_PSRAM` 未定义 | `apl_console.c` 中该宏求值为 0，控制台任务栈放内部 RAM（行为正确）；如需放 PSRAM 定义该宏为 1 |
| 5 | `board.h` 的 4 个扩展函数依赖 **本地修改版 FreeRTOS 内核** | 换用原版 IDF 后 `tasks/ps` 命令与 CPU 监控链接失败，需迁移内核补丁（见 §5） |

---

## 8. 构建与运行方式

### 8.1 环境要求

- ESP-IDF **v5.5.5**（本项目使用该版本及配套工具链）
- 本仓库需位于 `examples/idf_v555_my_exps/` 下（共享组件路径依赖）

### 8.2 首次配置

模板 `sdkconfig.defaults` 已指定 **esp32p4** 目标，无需手工 set-target；首次构建会自动生成 `sdkconfig`：

```powershell
idf.py build
```

（如需要强制重置目标配置：`idf.py set-target esp32p4`）

### 8.3 配置、编译、烧录、监控

```powershell
idf.py menuconfig        # 可选：调整 APP_ENABLE_CONSOLE / CONSOLE_STORE_HISTORY 等
idf.py build             # 编译
idf.py -p COMx flash     # 烧录（按实际串口修改 COMx）
idf.py -p COMx monitor   # 串口监视（板卡 UART：GPIO37/38，波特率 961200）
```

> 若启用 `CONSOLE_STORE_HISTORY`，请先按 README 在 `partitions_example.csv` 中补 `storage`(FAT) 分区。

### 8.4 运行效果

- 开机打印：芯片信息、Flash 信息、堆信息（来自 `app_info_dump`）。
- 控制台提示符：`<目标芯片名>>`（如 `esp32p4>`），支持 `help`、Tab 补全、上下键历史。
- 常用命令示例：

```
esp32p4> version      # 查看 IDF 版本
esp32p4> tasks        # 查看各任务栈/CPU 占用（依赖 FreeRTOS 内核扩展）
esp32p4> free         # 查看空闲堆
esp32p4> heap         # 查看堆统计
esp32p4> reboot       # 重启设备
# wifi / nvs / sleep 命令已按基础工程要求关闭，需要时在 board_config.h 打开
```

---

## 9. 如何基于本模板新建项目（快速上手）

1. 复制 `Template/base_template_p4` 为你的新工程目录（或直接在其上修改）。
2. 在 `board_config.h` 中按目标板填写引脚宏（LED/KEY/外设）与任务参数。
3. 需要控制台时保持 `APP_ENABLE_CONSOLE=y`；需要命令历史时补 `storage` 分区并开启 `CONSOLE_STORE_HISTORY`。
4. 在 `main.c` 的 `app_init()` 中挂接自己的业务初始化；业务任务建议用独立 Task 创建。
5. 编译前确认根 `CMakeLists.txt` 中 `idf_v55x_my_exps` 路径与实际目录一致。

---

*生成时间：2026-09-30 · 基于仓库当前代码状态整理*
