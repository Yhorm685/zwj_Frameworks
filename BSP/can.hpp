#pragma once

#include "BSP/transfer_mode.hpp"
#include "Common/status.hpp"

namespace fw::can {
struct Frame { uint16_t id; uint8_t length; uint8_t data[8]; };
constexpr bool validFrame(const Frame& f) { return f.id <= 0x7FF && f.length <= sizeof f.data; }
Status configure(bool loopback, TransferMode mode);
Status startSend(const Frame& frame, Ticket& ticket, uint32_t timeoutMs);
Status waitSent(Ticket ticket);
Status cancel(Ticket ticket);
Status send(const Frame& frame, uint32_t timeoutMs);
Status receive(Frame& output, uint32_t timeoutMs);
Status recover();
}
