#include "BSP/bringup.hpp"
#include "usart.h"
#include <cstring>

namespace fw {
bool writeBootLine(const char* message) {
    const auto length = std::strlen(message);
    return HAL_UART_Transmit(&huart1, reinterpret_cast<const uint8_t*>(message),
                             static_cast<uint16_t>(length), 50) == HAL_OK;
}
}
