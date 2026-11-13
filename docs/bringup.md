# F103 首次上板

## 接线与下载

先核对板载 HSE 标注为 8 MHz，BOOT0 置低，开发板正常供电。J-Link 通过 SWD 接板：VTref 接板上 3.3 V 用于检测目标电压，GND 共地，SWDIO 接 PA13，SWCLK 接 PA14，有复位脚时接 NRST。USB-TTL 使用 3.3 V 逻辑，RX 接 PA9、TX 接 PA10、GND 共地。开发板已有供电时只连串口信号和地。

在基线提交下构建 Debug ELF，用板卡目录中的 Ozone 工程打开 ELF，通过 J-Link 下载。Ozone 选择 SWD，复位后按 F5 连续运行。串口先打开 115200、8N1、无流控，再复位一次以捕获首行。本次不用接传感器、显示屏、舵机或 CAN 收发器。

| 用例 | 预期 | 实测 |
| --- | --- | --- |
| J-Link 下载与校验 | 目标识别正确，Ozone 下载完成 | 待回传 |
| 复位与调度 | 首行 F103 BASELINE READY，此后约每秒一行 TICK 1s | 待回传 |
| 断点检查 | 在 Framework_Run 内暂停可读取 fwHeartbeat；连续运行后再暂停应递增，fwBootTxErrors 为 0 | 待回传 |

请回传板卡型号、晶振标识、Ozone 的连接/下载结果，以及从复位开始至少 5 秒的串口原始文本。出现异常时保留原有输出、PC 所在函数或 Ozone 报错。调试器暂停期间的时间间隔不计入周期测量。

## 首次反馈 · COM4 收到 0 字节

本次比较对象为步骤 2 的源码 `57aa484` 和交付时已构建的 Debug ELF。操作者回传的 Ozone 输出和串口截图摘录如下。日志没有载明所打开的 ELF 路径，这次连接的固件版本待确认。

| 项目 | 实际记录 |
| --- | --- |
| J-Link | SWD 已连接，识别 Cortex-M3 r1p1。Flash 检查报告 `Skipped. Contents already match`，本次跳过写入。 |
| Ozone | 加载的 ELF 给出的 PC 为 `0x08002924`，显示 `Startup complete (PC=0x0800267E)` 后执行了 Continue。 |
| 串口 | COM4 已打开，115200、无校验、8 数据位、1 停止位。截图接收计数为 0，接收区空白。[串口截图](evidence/f103-first-com4.png) |
| 验收 | 已观察到调试器连接；尚未观察到基线的启动行或周期心跳。 |

### 固件和配置核对

步骤 1 仅配置时钟、SWD 和 HAL 时基。从步骤 2 开始，`boards/f103/f103.ioc` 中的 USART1 为 Asynchronous，PA9/PA10 分别是 TX/RX。生成的 main.c 在启动调度器前调用 MX_USART1_UART_Init，usart.c 打开 USART1 时钟、配置引脚并设置 115200、8N1。基线调用 HAL_UART_Transmit 进行阻塞发送；步骤 3 才需要 USART1 全局中断。

交付时基线 ELF 的 SHA-256 为 `937139324825931D9B8358459CFFD722A62B067EF35531068D1F912C9A022B98`，复位向量字为 `0x08002A99`，复位 PC 为 `0x08002A98`。这与反馈日志中的 ELF 入口不同。其他环境重新构建相同源码时地址也可能不同，以上数值用于识别本次交付的二进制文件。

### 复测

1. 在 Ozone 用 File → Open Project 打开步骤 2 的 f103.jdebug。它以工程目录为基准加载 build/Debug/f103.elf。用同目录的 f103.ioc 核对 USART1。保留已打开的 COM4，核对 PA9→USB-TTL RX、GND→GND。
2. 在 Framework_Run 设置断点，下载并运行。命中后取消断点，连续运行约 5 秒再暂停，记录 fwHeartbeat 和 fwBootTxErrors，同时保存串口文本。心跳增长且错误数为零说明固件执行过循环且 HAL 报告发送成功，此时继续验证 PA9 到 COM4 的物理通路。
3. 若断点未命中，记录 Ozone 当前函数、PC 和 framework_fault_line（非零时一并记录 framework_fault_file）。如果端口开始收到数据，请保存启动行与连续心跳，附上本次实际加载的工程或 ELF 路径。
