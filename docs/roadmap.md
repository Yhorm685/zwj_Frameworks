# 实施路线与日志

| 步骤 | 工作 | CubeMX 配置 | 验收 | 当前状态 |
| --- | --- | --- | --- | --- |
| 1 | 建立 F103 时钟与调试接口 | RCC、SWD、TIM4 HAL 时基 | CubeMX 生成 C 工程，核对时钟树并链接 ELF | 已生成并构建 |
| 2 | 建立 F103 调度与串口基线 | USART1、CMSIS-RTOS v2、Comm 栈和优先级 | C++17 入口、Ozone，串口观察启动和心跳 | 待实施 |
| 3 | 封装 UART 与统一日志 | USART1 全局中断 | 阻塞和中断收发、Busy、坏校验、半帧超时与恢复 | 待实施 |
| 4 | 封装定时器与两路舵机 | TIM2、中断、TIM3 两路 PWM 和引脚 | 核对 1 ms 计数、两路 50 Hz PWM 与独立动作 | 待实施 |
| 5 | 接入 I²C 和 MPU6050 | I²C1、事件/错误 IRQ、Sensor/Monitor、两个消息队列 | 两种读写方式、量程换算、设备断开后恢复 | 待实施 |
| 6 | 接入七针 OLED | SPI1、SPI 中断、CS/DC/RES 输出和初始电平 | 两种刷新方式；检查边框、四角与采样条形图 | 待实施 |
| 7 | 封装并验证 CAN | bxCAN、引脚、500 kbit/s 位时序、收发与错误 IRQ | 阻塞和中断回环、USB-CAN 双向实线和无应答 | 待实施 |
| 8 | 移植至 F407 并整理交付 | F407 的 RCC/SWD、USART1、TIM2/TIM4 和 Comm | 两板干净检出构建、再生成、F407 运行相同的 UART/TIM 验收 | 待实施 |

## 步骤 1 · 建立 F103 时钟与调试接口

从 STM32F103C8T6 新工程起步。8 MHz HSE 经 PLL ×9 得到 72 MHz，APB1 为 36 MHz，APB2 为 72 MHz；保留 PA13/PA14 的 SWD。TIM4 提供 HAL tick。Debug 已交叉编译，见本阶段 .ioc 和生成的 SystemClock_Config。
