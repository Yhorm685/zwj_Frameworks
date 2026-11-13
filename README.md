# Helios Frameworks

任务 01 的电控框架。F103C8T6 主板运行 MPU6050 采样、OLED 显示、两路 SG90、CAN 及串口控制。F407ZGT6 复用串口和定时器实现，用于验证最小移植。板级配置由 STM32CubeMX 管理，应用使用 C++17 和 FreeRTOS。

## 构建与调试

安装 Arm GNU Toolchain（含 g++）、CMake 和 Ninja，加入 PATH。在仓库根目录执行以下命令。将 f103 换为 f407 可构建移植板。

```powershell
cd boards/f103
cmake --preset Debug
cmake --build --preset Debug
```

在同目录打开对应的 .jdebug，Ozone 通过 J-Link/SWD 加载 build/Debug 下的 ELF。F103 和 F407 的 USART1 均用 PA9/TX、PA10/RX，115200、8N1。检查晶振、供电和接线后上板。F103 固定基线 `57aa484` 的输出是 `F103 BASELINE READY` 和 `TICK 1s`，当前集成固件输出 `[F103][Ok] framework ready`，两者请按提交号区分。

## 文档

| 内容 | 入口 |
| --- | --- |
| 题目、进度和参考 | [需求清单](docs/requirements.md) · [roadmap 和阶段日志](docs/roadmap.md) · [跃鹿参考分析](docs/reference.md) |
| 架构、接口和任务 | [架构与工程目录](docs/architecture.md) |
| 调试与验收 | [F103 基线](docs/bringup.md) · [测试命令与证据](docs/validation.md) · [F407 移植](docs/port.md) |
| 外设与模块 | [UART](docs/uart.md) · [Timer/SG90](docs/timer-servo.md) · [I²C/MPU6050](docs/i2c-mpu6050.md) · [SPI/OLED](docs/spi-oled.md) · [CAN](docs/can.md) |
| 对话记录 | [AI Session](docs/ai-session.md) |

交叉编译、主机检查和实物运行的结果分别列入验证记录。两块板的串口原始记录、USB-CAN 抓包和演示视频随实际测试补入仓库。
