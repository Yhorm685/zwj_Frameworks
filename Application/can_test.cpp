#include "Application/can_test.hpp"
#include "BSP/can.hpp"
#include <cstdio>
#include <cstring>

namespace fw::app {
static void describe(Logger& log, const can::Frame& f) {
    char line[60];
    int offset = std::snprintf(line, sizeof(line), "id=%03X dlc=%u data=", f.id, f.length);
    for (unsigned i = 0; i < f.length; ++i)
        offset += std::snprintf(line + offset, sizeof(line) - offset, "%02X", f.data[i]);
    logResult(log, "CAN", Status::Ok, line);
}

bool canCommand(Frame frame, Logger& log) {
    if (frame.command < 0x40 || frame.command > 0x44) return false;
    Status result = Status::InvalidArgument;
    const can::Frame outgoing{0x321, 4, {0xA5, frame.argument, 0x5A, 0xC3}};
    can::Frame incoming{};
    switch (frame.command) {
    case 0x40:
        if (frame.argument < 4)
            result = can::configure(frame.argument < 2, frame.argument & 1 ? TransferMode::Interrupt : TransferMode::Blocking);
        break;
    case 0x41:
        result = can::send(outgoing, 100);
        if (result == Status::Ok) describe(log, outgoing);
        break;
    case 0x42:
        result = can::receive(incoming, 200);
        if (result == Status::Ok) describe(log, incoming);
        break;
    case 0x43:
        result = can::send(outgoing, 100);
        if (result == Status::Ok) result = can::receive(incoming, 100);
        if (result == Status::Ok) {
            describe(log, incoming);
            if (incoming.id != outgoing.id || incoming.length != outgoing.length ||
                std::memcmp(incoming.data, outgoing.data, outgoing.length) != 0) result = Status::DeviceMismatch;
        }
        break;
    case 0x44:
        result = can::recover();
        break;
    }
    logValue(log, "CAN", result, "cmd", frame.command);
    return true;
}
}
