# 实施路线与日志

| 步骤 | 工作 | CubeMX 配置 | 验收 | 当前状态 |
| --- | --- | --- | --- | --- |
| 1 | 建立 F103 时钟与调试接口 | RCC、SWD、TIM4 HAL 时基 | CubeMX 生成 C 工程，核对时钟树并链接 ELF | 已生成并构建 |
| 2 | 建立 F103 调度与串口基线 | USART1、CMSIS-RTOS v2、Comm 栈和优先级 | C++17 入口、Ozone，串口观察启动和心跳 | 软件已构建，实机待验 |
| 3 | 封装 UART 与统一日志 | USART1 全局中断 | 阻塞和中断收发、Busy、坏校验、半帧超时与恢复 | 软件已构建，实机待验 |
| 4 | 封装定时器与两路舵机 | TIM2、中断、TIM3 两路 PWM 和引脚 | 核对 1 ms 计数、两路 50 Hz PWM 与独立动作 | 软件已构建，实机待验 |
| 5 | 接入 I²C 和 MPU6050 | I²C1、事件/错误 IRQ、Sensor/Monitor、两个消息队列 | 两种读写方式、量程换算、设备断开后恢复 | 软件已构建，实机待验 |
| 6 | 接入七针 OLED | SPI1、SPI 中断、CS/DC/RES 输出和初始电平 | 两种刷新方式；检查边框、四角与采样条形图 | 待实施 |
| 7 | 封装并验证 CAN | bxCAN、引脚、500 kbit/s 位时序、收发与错误 IRQ | 阻塞和中断回环、USB-CAN 双向实线和无应答 | 待实施 |
| 8 | 移植至 F407 并整理交付 | F407 的 RCC/SWD、USART1、TIM2/TIM4 和 Comm | 两板干净检出构建、再生成、F407 运行相同的 UART/TIM 验收 | 待实施 |

## 步骤 1 · 建立 F103 时钟与调试接口

从 STM32F103C8T6 新工程起步。8 MHz HSE 经 PLL ×9 得到 72 MHz，APB1 为 36 MHz，APB2 为 72 MHz；保留 PA13/PA14 的 SWD。TIM4 提供 HAL tick。Debug 已交叉编译，见本阶段 .ioc 和生成的 SystemClock_Config。

## 步骤 2 · 建立 F103 调度与串口基线

在 CubeMX 中配置 USART1 PA9/PA10（115200、8N1）和 Comm（Normal、256 words）。SysTick 供 FreeRTOS，TIM4 保留给 HAL。J-Link/Ozone 与用户 C++ 任务入口已接入。首次上板只需主板、J-Link 和串口模块，见 [基线测试](bringup.md)。

## 步骤 3 · 封装 UART 与统一日志

通过 .ioc 启用 USART1 IRQ。接收中断搬运字节并保存到达时间；单一通信任务完成校验与日志输出。主机异常覆盖及板上命令见 [串口验证](uart.md)。

## 步骤 4 · 封装定时器与两路舵机

TIM2 配为 1 MHz 计数、ARR=999；TIM3 配为 1 MHz 计数、ARR=19999，CH3/CH4 分别输出到 PB0/PB1。两种更新确认与测试动作见 [定时器与舵机](timer-servo.md)。

## 步骤 5 · 接入 I²C 和 MPU6050

在 CubeMX 的 FreeRTOS Tasks and Queues 页面配置三项任务及两项消息队列。CubeMX 生成对象、容量、存储、优先级和栈；应用负责采样循环、启动同步和错误处理。[传感器验证](i2c-mpu6050.md)
