# Helios Frameworks

F103C8T6 为主板，板级配置保存在 boards/f103/f103.ioc。进入 boards/f103 后运行 `cmake --preset Debug`、`cmake --build --preset Debug`，再从同目录的 Ozone 工程使用 J-Link/SWD 加载 ELF。串口 PA9/TX、PA10/RX，115200、8N1。

[实施路线](docs/roadmap.md) · [首次上板](docs/bringup.md) · [题目清单](docs/requirements.md) · [参考分析](docs/reference.md)
