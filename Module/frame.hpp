#pragma once

#include "Common/status.hpp"

namespace fw {
struct Frame {
    std::uint8_t command{};
    std::uint8_t argument{};
};

// A5 CMD ARG SUM8(A5, CMD, ARG). A5 is legal inside either payload byte.
class FrameParser {
public:
    static constexpr std::uint8_t header = 0xA5;
    static constexpr std::uint32_t gapMs = 20;
    void reset() { count_ = 0; }

    Status expire(std::uint32_t nowMs) {
        if (count_ && nowMs - lastByte_ >= gapMs) {
            count_ = 0;
            return Status::Timeout;
        }
        return Status::Busy;
    }

    Status push(std::uint8_t byte, std::uint32_t nowMs, Frame& out) {
        const Status gap = expire(nowMs);
        lastByte_ = nowMs;
        switch (count_) {
        case 0:
            count_ = byte == header ? 1 : 0;
            return gap;
        case 1:
            pending_.command = byte;
            ++count_;
            break;
        case 2:
            pending_.argument = byte;
            ++count_;
            break;
        default:
            if (byte == static_cast<std::uint8_t>(header + pending_.command + pending_.argument)) {
                out = pending_;
                count_ = 0;
                return Status::Ok;
            }
            // A missing checksum can put the following frame's header here.
            count_ = byte == header ? 1 : 0;
            return Status::ChecksumError;
        }
        return Status::Busy;
    }
private:
    Frame pending_{};
    std::uint32_t lastByte_{};
    std::uint8_t count_{};
};
}
