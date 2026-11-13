#pragma once

#include "BSP/transfer_mode.hpp"
#include "Common/status.hpp"
#include <cstddef>

namespace fw::spi {
// SPI1 display port; data=false selects commands. The port owns CS, DC and RES.
Status writeDisplay(bool data, const uint8_t* bytes, size_t size, TransferMode mode, uint32_t ms = 30);
Status startDisplay(bool data, const uint8_t* bytes, size_t size, Ticket& ticket, uint32_t ms = 30);
Status wait(Ticket ticket);
Status cancel(Ticket ticket);
Status resetDisplay();
Status recover();
}
