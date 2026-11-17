# F407ZGT6 最小移植

1. F407 工程与 F103 分开生成，使用 STM32CubeF4 V1.28.3。根据实际板卡确认 HSE；当前工程配置为 8 MHz 外部晶振，经 PLL 得到 168 MHz。若板卡使用其他晶振，先在 CubeMX 修改频率和 PLL，核对 APB 和定时器时钟，再生成并重建。
2. 在 CubeMX 保留 SWD、USART1 PA9/PA10、TIM2、TIM4 HAL 时基及 FreeRTOS。TIM2 设置为 1 MHz 计数、ARR=999。F407 的 APB1 定时器时钟与 F103 不同，保持 1 ms 更新周期需要不同的预分频。
3. 从 boards/f407 执行 README 的 Debug 构建。该目标提供 FRAMEWORK_F407，复用 BSP 的 UART、TIM2 和带校验帧的命令处理。确认 .jdebug 选择 STM32F407ZG，从本板构建目录加载 ELF。
4. 用 J-Link/SWD 烧录，USB-TTL 接 USART1。启动后运行当前串口 smoke，保存回显与故障恢复应答。在 Ozone 中记录共用定时器的 `ticks_` 和两次读数间的实际运行时间，按 F103 的同一标准验收。[TIM2 观察方法](timer-servo.md)
5. 将接线、实测晶振、固件 commit、Ozone 下载日志与串口输出附入本仓库。若需移植到第三种型号，先生成该板硬件配置及入口，再复用相同 BSP 接口和本组验收命令。

## 重新生成

用 STM32CubeMX 6.18.1 打开要修改的 .ioc，选择对应本地固件包（F103 用 F1 V1.8.7，F407 用 F4 V1.28.3）并 Generate Code。保持板目录既有的 CMakeLists.txt，应用入口和故障钩子放在 CubeMX 的 USER CODE 区。生成后对照 Git diff 并从板目录重跑 Debug 配置与构建。

用 CubeMX 打开 boards 下对应的 .ioc。在 FreeRTOS → Tasks and Queues 中，F103 可见 Comm、Sensor、Monitor 和两条队列，F407 只需 Comm。再次生成后核对生成的任务/队列创建、C++ 桥接和中断入口，并重新构建。.mxproject 为本机生成文件。

本次在无 .mxproject 的隔离目录重新运行两板的 CubeMX 生成，两个工程原有的 286 个非构建文件 SHA-256 逐一相同。
