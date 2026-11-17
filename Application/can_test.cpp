#include "Application/can_test.hpp"
#include "BSP/can.hpp"
#include <cstdio>
#include <cstring>

namespace fw::app {
static void logFrame(Logger& log, Status status, const char* action, const can::Frame& frame) {
    char line[60];
    int offset = std::snprintf(line, sizeof(line), "%s id=%03X dlc=%u data=", action,
        static_cast<unsigned>(frame.id), static_cast<unsigned>(frame.length));
    for (unsigned i = 0; i < frame.length; ++i)
        offset += std::snprintf(line + offset, sizeof(line) - offset, "%02X",
            static_cast<unsigned>(frame.data[i]));
    logResult(log, "CAN", status, line);
}

bool canCommand(Frame frame, Logger& log) {
    const can::Frame outgoing{0x321, 4, {0xA5, frame.argument, 0x5A, 0xC3}};
    can::Frame incoming{};
    const can::Frame* report = nullptr;
    const char* action;
    Status result = Status::InvalidArgument;

    switch (frame.command) {
    case 0x40:
        action = "configure";
        if (frame.argument < 4)
            result = can::configure(frame.argument < 2, frame.argument & 1 ? TransferMode::Interrupt : TransferMode::Blocking);
        break;
    case 0x41:
        action = "tx";
        result = can::send(outgoing, 100);
        if (result == Status::Ok) report = &outgoing;
        break;
    case 0x42:
        action = "rx";
        result = can::receive(incoming, 200);
        if (result == Status::Ok) report = &incoming;
        break;
    case 0x43:
        action = "loopback tx";
        result = can::send(outgoing, 100);
        if (result != Status::Ok) break;
        action = "loopback rx";
        result = can::receive(incoming, 100);
        if (result != Status::Ok) break;
        action = "loopback";
        report = &incoming;
        if (incoming.id != outgoing.id || incoming.length != outgoing.length ||
            std::memcmp(incoming.data, outgoing.data, outgoing.length) != 0)
            result = Status::DeviceMismatch;
        break;
    case 0x44:
        action = "recover";
        result = can::recover();
        break;
    default:
        return false;
    }

    if (report) logFrame(log, result, action, *report);
    else logResult(log, "CAN", result, action);
    return true;
}
}
