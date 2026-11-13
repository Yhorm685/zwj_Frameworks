#include "Application/display.hpp"
#include "Application/sensors.hpp"
#include "Module/oled.hpp"

namespace fw::app {
bool displayCommand(Frame frame, Logger& log) {
    if (frame.command != 0x30 && frame.command != 0x31) return false;
    Status result = Status::InvalidArgument;
    if (frame.argument <= 1) {
        const auto mode = frame.argument ? TransferMode::Interrupt : TransferMode::Blocking;
        if (frame.command == 0x30) result = oled::showPattern(mode);
        else {
            mpu6050::Sample sample{};
            result = latestSample(sample) ? oled::showSample(sample.ax, mode) : Status::NotInitialized;
        }
    }
    logResult(log, "OLED", result, frame.command == 0x30 ? "test pattern" : "latest ax");
    return true;
}
}
