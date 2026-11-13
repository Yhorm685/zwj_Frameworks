# 跃鹿参考框架分析

参考版本为 master 的 `27b903b615b3bf8bbb7b3f404baf0b36ed6d3080`。上游以 F407IGHx 为目标，分为 application、modules 和 bsp，硬件初始化由 CubeMX 管理。[上游仓库](https://github.com/HNUYueLuRM/basic_framework/tree/27b903b615b3bf8bbb7b3f404baf0b36ed6d3080)

| 观察 | 本工程采用的做法 | 理由 |
| --- | --- | --- |
| UART 以外设实例分派 HAL 回调 | 板级代码持有 HAL 句柄，BSP 区分收发方向，业务通过接口取完成状态 | 并行收发时能追踪缓冲区所有权和最终结果 |
| I²C 将设备地址与控制器关联 | 公共接口统一使用七位地址，在 HAL 边界左移；设备驱动处理寄存器、身份和换算 | 同一设备可复用到另一块板，避免混用两种地址记法 |
| CAN 集中过滤器、FIFO 和邮箱回调 | 验收时比对报文内容并观察邮箱最终状态，分别记录内部回环和实线收发 | 邮箱可用、写入成功和总线应答是不同事件 |
| 部分注册路径使用动态分配，部分错误路径永久等待 | 使用有界存储、限时等待、显式状态；早期启动故障可在 Ozone 检查 | F103 片上 SRAM 有限，通信异常后需可诊断、可恢复 |

硬件引脚、时钟、启动文件与链接脚本按本项目两块 MCU 生成。上游业务算法不属于任务 01 本次范围。[UART](https://github.com/HNUYueLuRM/basic_framework/blob/27b903b615b3bf8bbb7b3f404baf0b36ed6d3080/bsp/usart/bsp_usart.c) · [I²C](https://github.com/HNUYueLuRM/basic_framework/blob/27b903b615b3bf8bbb7b3f404baf0b36ed6d3080/bsp/iic/bsp_iic.c) · [CAN](https://github.com/HNUYueLuRM/basic_framework/blob/27b903b615b3bf8bbb7b3f404baf0b36ed6d3080/bsp/can/bsp_can.c) · [上游许可证](https://github.com/HNUYueLuRM/basic_framework/blob/27b903b615b3bf8bbb7b3f404baf0b36ed6d3080/LICENSE)
