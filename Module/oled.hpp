#pragma once

#include "BSP/transfer_mode.hpp"
#include "Common/status.hpp"

namespace fw::oled {
Status showPattern(TransferMode mode);
Status showSample(int16_t ax, TransferMode mode);
}
