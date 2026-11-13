# 串口协议与验证

USART1 为 115200、8N1。每个命令为四字节 `A5 CMD ARG SUM`，SUM 取前三字节相加的低八位。例如回送 17 发送 `A5 01 11 B7`。回复为可直接保存的 ASCII 行。常规接收使用中断环形缓冲区，每个字节带有到达时间；相邻字节间隔达到 20 ms 时丢弃未完成帧。

| CMD | 作用 | 观察 |
| --- | --- | --- |
| 01 | 回送 ARG | ECHO、Ok 和参数值 |
| 02 | 中断发送并尝试提交第二笔 | IT TX、Busy 和第一笔最终结果 |
| 03 / 04 | 暂停命令接收，以阻塞 / 定长中断方式接收单字节 | 收到提示后 500 ms 内发一个字节，或留空以触发超时；随后恢复命令接收 |
| 05 | 查询通信任务 | 剩余栈字节数、日志发送失败次数 |

PC 端装有 pyserial 时，运行 `python tools/console.py COM5 smoke`。它会检查连续两帧、坏校验、半帧超时、中断发送、两种接收的成功和超时，以及出错后的下一帧。将 COM5 换为实际串口，先关闭占用端口的串口助手。单发示例为 `python tools/console.py COM5 0x01 17`。端口收发、脚本输出和实际固件版本应一起留存。

## 步骤 3 独立固件

版本为 `6a420dc`。打开 [UART Ozone 工程](../boards/f103/f103-uart.jdebug)，它加载 `boards/f103/build/uart/f103.elf`。F103 的接线沿用基线；PC 发出的字节由 USB-TTL TX 输入到 PA10。串口助手选择 COM4、115200、8N1，勾选“十六进制发送”，接收使用文本显示，单次发送，不追加回车换行。

| 顺序 | 发送的十六进制字节 | 预期串口文本 |
| --- | --- | --- |
| 1 | 打开串口，下载、复位并运行 | `[F103][Ok] framework ready` |
| 2 | `A5 01 11 B7` | `[ECHO][Ok] arg=17` |
| 3 | `A5 01 11 B6` | `[FRAME][ChecksumError] receive` |
| 4 | 再发 `A5 01 11 B7` | 再次收到 `[ECHO][Ok] arg=17`，确认坏帧后恢复 |
| 5 | `A5 01`，等待超过 20 ms | `[FRAME][Timeout] partial frame` |
| 6 | `A5 02 00 A7` | 依次输出 `IT TX`、`[UART][Busy] second start`、`[UART][Ok] interrupt TX final` |

请保留复位首行和各次命令的原始回包。全部测试还包括阻塞及定长中断接收的成功、超时和恢复；安装有 pyserial 时可关闭串口助手，用仓库根目录的 `python tools/console.py COM4 smoke` 自动执行。

从新检出的仓库复现独立构建时，在仓库根目录运行下列 PowerShell 命令；首次建立源码工作树后，后续可直接重新构建。

```powershell
git worktree add --detach build/uart-src 6a420dc
$uartOutput = Join-Path (Get-Location) 'boards/f103/build/uart'
Push-Location build/uart-src/boards/f103
cmake --preset Debug -B $uartOutput
cmake --build $uartOutput
Pop-Location
```
