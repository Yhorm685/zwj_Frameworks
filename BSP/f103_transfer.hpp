#pragma once

#include "BSP/transfer_mode.hpp"
#include "Common/status.hpp"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <atomic>

namespace fw::detail {
// Task-side gates never spin. Interrupt handlers only publish hardware results.
class Gate {
    std::atomic_flag& flag_;
    bool acquired_;
public:
    explicit Gate(std::atomic_flag& flag) : flag_(flag), acquired_(!flag.test_and_set()) {}
    ~Gate() { if (acquired_) flag_.clear(); }
    explicit operator bool() const { return acquired_; }
    Gate(const Gate&) = delete;
    Gate& operator=(const Gate&) = delete;
};

inline Status status(HAL_StatusTypeDef result) {
    switch (result) {
        case HAL_OK: return Status::Ok;
        case HAL_BUSY: return Status::Busy;
        case HAL_TIMEOUT: return Status::Timeout;
        default: return Status::BusError;
    }
}

inline bool validTimeout(uint32_t ms) { return ms > 0 && ms <= INT32_MAX; }
inline bool expired(uint32_t start, uint32_t duration) { return HAL_GetTick() - start >= duration; }
inline void nextTick() { vTaskDelay(1); }

// Each peripheral has its own ticket sequence. Call under the peripheral gate.
struct Operation {
    uint32_t sequence{}, start{}, duration{};
    bool active{};
    void arm(Ticket& ticket, uint32_t ms) {
        if (++sequence == 0) ++sequence;
        ticket.value = sequence;
        duration = ms;
        start = HAL_GetTick();
        active = true;
    }
    bool owns(Ticket ticket) const { return active && ticket.value == sequence; }
    bool timedOut() const { return expired(start, duration); }
};
}
