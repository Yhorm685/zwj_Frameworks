#include "BSP/bringup.hpp"
#include "cmsis_os2.h"
#include <cstdint>

extern "C" {
volatile uint32_t fwHeartbeat = 0;
volatile uint32_t fwBootTxErrors = 0;

void Framework_Run() {
    if (!fw::writeBootLine("F103 BASELINE READY\r\n")) ++fwBootTxErrors;
    for (;;) {
        ++fwHeartbeat;
        if (!fw::writeBootLine("TICK 1s\r\n")) ++fwBootTxErrors;
        osDelay(osKernelGetTickFreq());
    }
}
}
