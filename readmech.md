# HiBean ESP32 BLE Skywalker v1 烘焙机控制器

本项目为 ESP32 系列开发板实现了 Skywalker Community Edition（Skywalker Comm）烘焙机控制器，已在 Waveshare ESP32-S3-Zero 和 Espressif ESP32C6-devkitC 上测试，并连接到 Skywalker v1 烘焙机使用。请注意：本固件不适用于 Skywalker v2。

[HiBean](https://www.hibean.fun/en/) 烘焙团队支持并维护了一个面向社区的 SkyWalker 控制器版本。该版本基于 Arduino 和 ESP32 实现，采用开源 GPLv3 协议，你可以自由 fork、修改，也可以直接使用每个 [Release](https://github.com/MagnmCI/SkiBeanCommunity/releases) 中提供的预编译二进制固件。

特别感谢 jmoore52、mrnoone5313 和 Nirecue 在 [Skywalker Roaster Labs](https://github.com/jmoore52/SkywalkerRoaster) 中完成的优秀工作，本版本正是受他们的项目启发而来。如果你想参与讨论，可以加入 [HiBean Discord 服务器](https://discord.gg/pEM8MkY9)。

## 可用二进制固件（.bin）

每当项目完成一个适合发布的阶段性功能集时，都会生成一个 Release，并自动为受支持的 ESP32 平台构建预编译 `.bin` 固件。

你可以使用常用的 [ESP32 刷写工具](https://web.esphome.io/) 写入这些 `.bin` 文件。刷写完成后，板载 LED 会在红色和蓝色之间交替闪烁，直到它与 HiBean 等 BLE 客户端完成配对。

如果需要重新刷写已有的 S3-Zero，请在拔下并重新插入 USB 时按住 BOOT 按钮，使设备进入 DFU 模式，然后再写入新固件。

## IDE / 构建说明

本仓库适合使用 VSCode 的 PlatformIO 风格扩展进行开发。不过请特别使用 [`pioarduino`](https://github.com/pioarduino) 这个 PlatformIO 分支，因为它包含了面向 Arduino 开发所需的较新 ESP32 板卡定义。

开发环境准备步骤：

1. 安装 VSCode。
2. 安装 pioarduino 扩展。
3. 将本仓库 checkout 到本地目录。
4. 在 pioarduino 中选择打开已有项目，并选择本仓库中的 `platformio.ini` 文件。

随后，所需的开发环境、依赖库和工具链会自动安装并配置完成，你就可以开始构建和刷写固件了。

## ESP32-C3 Super Mini 接线说明

`esp32-c3-super-mini` 这个 PlatformIO 环境配置了两个 MAX6675 热电偶模块。两个 MAX6675 模块共用 SCK 和 SO，每个模块使用独立的 CS 引脚。

| 信号 | ESP32-C3 Super Mini GPIO | 说明 |
|------|---------------------------|------|
| MAX6675 #1 SCK | GPIO4 | 共用时钟。 |
| MAX6675 #1 SO | GPIO6 | 两个 MAX6675 模块共用的数据输出。 |
| MAX6675 #1 CS | GPIO5 | 第一个热电偶模块的片选。 |
| MAX6675 #2 SCK | GPIO4 | 共用时钟。 |
| MAX6675 #2 SO | GPIO6 | 两个 MAX6675 模块共用的数据输出。 |
| MAX6675 #2 CS | GPIO10 | 第二个热电偶模块的片选。 |

这些值可以在 `platformio.ini` 中通过 `MAX6675_SO_PIN`、`MAX6675_2_SO_PIN` 和 `MAX6675_2_CS_PIN` 修改。固件默认使用 GPIO5 作为第一个 CS 引脚，GPIO4 作为共用 SCK 引脚。

在 ESP32-C3 Super Mini 上，GPIO19 不用于 MAX6675 的 SO。许多 ESP32-C3 Super Mini 开发板上的 GPIO18/GPIO19 是 USB D-/D+，而不是普通排针引脚。

烘焙机控制输出仍然使用 `platformio.ini` 中定义的 `TX_PIN`。如果你的 ESP32-C3 Super Mini 没有引出 GPIO18，请在接入烘焙机控制线之前，将 `TX_PIN` 修改为开发板上实际可用的 GPIO。

## 控制命令与行为

HiBean 和本烘焙机控制软件大体实现了 [TC4 命令](https://github.com/greencardigan/TC4-shield/blob/master/applications/Artisan/aArtisan/trunk/src/aArtisan/commands.txt)，用于支持大多数烘焙机功能。可用命令如下。

## 可用命令（大小写不敏感）

### 通用命令

| 命令 | 说明 |
|------|------|
| `OT2;XX` | 将排风功率设置为 **XX%**。 |
| `OFF` | 关闭系统。 |
| `ESTOP` | 紧急停止：加热器设为 0%，排风设为 100%。 |
| `DRUM;XX` | 启动或停止滚筒电机（1 = 开，0 = 关）。 |
| `FILTER;XX` | 控制过滤风扇功率（1 最快，4 最慢，0 关闭）。 |
| `COOL;XX` | 启用冷却功能（0-100%）。 |
| `CHAN` | 发送当前激活的通道配置。 |
| `UNITS;C/F` | 设置温度单位为 **摄氏度（C）** 或 **华氏度（F）**。 |

### PID 控制命令

| 命令 | 说明 |
|------|------|
| `PID;ON` | 启用 PID 控制（自动模式）。 |
| `PID;OFF` | 关闭 PID 控制（切换到手动模式）。 |
| `PID;SV;XXX` | 设置 PID **目标温度**，XXX 单位为摄氏度。例如 `PID;SV;250` 表示目标温度 250°C。 |
| `PID;T;PP.P;II.I;DD.D` | 应用给定的 PID 参数（不会持久保存）。 |
| `PID;CT;XXXX` | 临时设置 PID 循环/采样时间，单位为毫秒（不会持久保存）。 |
| `PID;PM;E` | 临时切换 pMode：E = P_ON_E，M = P_ON_M（默认）；也可反向切换（不会持久保存）。 |
| `OT1;XX` | PID 关闭时，手动设置加热器功率为 **XX%**；PID 开启时，设置最大加热功率限制。 |
| `READ` | 读取当前状态，返回格式为 `0,temp1,temp2,heater,vent`，其中 `temp1` 和 `temp2` 是两个 MAX6675 的读数。 |

## 使用示例

启用 PID 控制：

```text
PID;ON
```

设置目标温度为 250°C：

```text
PID;SV;250
```

手动设置加热器功率为 70%，或在 PID 模式下设置最大功率限制：

```text
OT1;70
```

读取当前系统状态：

```text
READ
```

从本版本开始以及后续版本中，PID 控制也会通过 BLE 暴露。具体细节可以查看 `SkiBLE` 头文件。这样做主要是因为 TC4 命令并不支持完整的 PID 控制命令集，也无法通过 TC4 读取当前状态，只能写入控制指令。

## 志愿维护说明

本代码库由志愿者维护。使用本软件时，请理解你需要自行承担调试和使用风险。你可以在仓库中提交 Issue，开发者会在时间允许时处理。

## 许可证

本项目采用 GNU General Public License v3.0（GPLv3）许可证。
