# Helios Frameworks

任务 01 的电控框架。F103C8T6 主板运行 MPU6050 采样、OLED 显示、两路 SG90、CAN 及串口控制。F407ZGT6 复用串口和定时器实现，用于验证最小移植。板级配置由 STM32CubeMX 管理，应用使用 C++17 和 FreeRTOS。

## 构建与调试

安装 Arm GNU Toolchain（含 g++）、CMake 和 Ninja，加入 PATH。在仓库根目录执行以下命令。将 f103 换为 f407 可构建移植板。

```powershell
cd boards/f103
cmake --preset Debug
cmake --build --preset Debug
```

F103 和 F407 的 USART1 均用 PA9/TX、PA10/RX，115200、8N1。两板集成固件在启动成功、空闲时保持串口安静。要确认串口双向通信，HEX 发送 `A5 01 11 B7`，预期收到 `[ECHO][Ok] arg=17`。后台故障会主动上报。固定基线 `57aa484` 保留其原有的 `TICK 1s` 心跳。

Ozone 通过 J-Link/SWD 加载各板 build/Debug 下的 ELF。[F103 集成工程](boards/f103/f103.jdebug) · [F407 移植工程](boards/f407/f407.jdebug) · [最终验收顺序](docs/validation.md#集中验收顺序)

## 文档

| 内容 | 入口 |
| --- | --- |
| 题目、进度和参考 | [需求清单](docs/requirements.md) · [roadmap 和阶段日志](docs/roadmap.md) · [跃鹿参考分析](docs/reference.md) |
| 架构、接口和任务 | [架构与工程目录](docs/architecture.md) |
| 调试与验收 | [F103 基线](docs/bringup.md) · [测试命令与证据](docs/validation.md) · [F407 移植](docs/port.md) |
| 外设与模块 | [UART](docs/uart.md) · [Timer/SG90](docs/timer-servo.md) · [I²C/MPU6050](docs/i2c-mpu6050.md) · [SPI/OLED](docs/spi-oled.md) · [CAN](docs/can.md) |
| 对话记录 | [AI Session](docs/ai-session.md) |

交叉编译、主机检查和实物运行的结果分别列入验证记录。两块板的串口原始记录、USB-CAN 抓包和演示视频随实际测试补入仓库。
