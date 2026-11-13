# 定时器与两路舵机

TIM2 计数频率 1 MHz，ARR=999，每 1 ms 更新一次。启动日志分别打印轮询等待与周期中断的结果。发送命令 10 两次，比较返回的 ticks 差及 PC 两次发送间隔；调试器暂停期间单独标记。在 F407 上复用这组命令。

SG90 信号为 PB0/TIM3_CH3 与 PB1/TIM3_CH4。TIM3 为 50 Hz，初始脉宽 1500 µs，本工程 0~180 指令映射为 1000~2000 µs。数字为标称指令，具体输出轴角度需用实物确认。舵机单独供电并与 F103 共地，先卸负载，检查回中和小角度，再装支架。固件上电保持 PWM 未启动，由命令明确启用。

| CMD（十六进制） | ARG（十进制） | 验收 |
| --- | --- | --- |
| 11 / 12 | 90，然后 80、100、90 | 依次操作 yaw / pitch，等待 PWM 更新，记录独立动作 |
| 13 / 14 | 90，然后 80、100、90 | 通过更新中断确认，观察角度方向和串口状态 |
| 11 / 12 | 181 或 255 | 前者返回 InvalidArgument，后者停止该通道 |

硬件测试请回传操作顺序、原始串口日志和两路舵机动作的视频。重复性和角度误差可在云台任务中另行测量。

## 步骤 4 独立固件

测试版本为 `1a6ed94`。在工程内打开 [定时器 Ozone 工程](../boards/f103/f103-timer.jdebug)，调试器会加载 `boards/f103/build/timer/f103.elf`。先保持两只舵机断开，只检查启动时的 TIM2 轮询、1 ms 周期中断和命令 10 的计数；保留 PC 的收发时间及两次 ticks。随后再单独接入舵机验证动作。

新检出仓库后可从根目录构建这一固定版本。首次建立工作树后，可以直接重新构建。

```powershell
git worktree add --detach build/timer-src 1a6ed94
$timerOutput = Join-Path (Get-Location) 'boards/f103/build/timer'
Push-Location build/timer-src/boards/f103
cmake --preset Debug -B $timerOutput
cmake --build $timerOutput
Pop-Location
```
