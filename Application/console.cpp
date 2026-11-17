#include "Application/commands.hpp"
#include "BSP/board.hpp"
#ifdef FRAMEWORK_F103
#include "Application/sensors.hpp"
#endif
#include "cmsis_os2.h"
#include <cstdio>

namespace fw::app {
namespace {
constexpr uint32_t consoleEvent = 1U;
constexpr uint32_t serviceEvent = 2U;
// Comm owns both the log sink and the counters for reported events.
uint32_t logFailures{};
#ifdef FRAMEWORK_F103
uint32_t busErrors{}, timeouts{}, checksumErrors{};
#endif

void wakeConsole(void* task) {
    osThreadFlagsSet(task, consoleEvent);
}

void reportHealth(Logger& log) {
    char message[80];
#ifdef FRAMEWORK_F103
    logWorkerResources(log);
    std::snprintf(message, sizeof(message), "bus=%lu timeout=%lu checksum=%lu log_write=%lu",
        static_cast<unsigned long>(busErrors), static_cast<unsigned long>(timeouts),
        static_cast<unsigned long>(checksumErrors), static_cast<unsigned long>(logFailures));
    logResult(log, "MON", Status::Ok, message);
#else
    std::snprintf(message, sizeof(message), "stack_free_B=%lu log_errors=%lu",
        static_cast<unsigned long>(osThreadGetStackSpace(osThreadGetId())),
        static_cast<unsigned long>(logFailures));
    logResult(log, "COMM", Status::Ok, message);
#endif
}

void exchange(Frame frame, Logger& log) {
    switch (frame.command) {
    case 0x01:
        logValue(log, "ECHO", Status::Ok, "arg", frame.argument);
        break;
    case 0x05:
        reportHealth(log);
        break;
    default:
        if (!boardCommand(frame, log)) logResult(log, "CMD", Status::InvalidArgument, "unknown command");
    }
}
}

void logResult(Logger& log, const char* source, Status status, const char* message) {
#ifdef FRAMEWORK_F103
    switch (status) {
    case Status::BusError: ++busErrors; break;
    case Status::Timeout: ++timeouts; break;
    case Status::ChecksumError: ++checksumErrors; break;
    default: break;
    }
#endif
    if (log.write(source, status, message, 50) != Status::Ok) ++logFailures;
}

void logValue(Logger& log, const char* source, Status status, const char* label, int32_t value) {
    char message[64];
    std::snprintf(message, sizeof(message), "%s=%ld", label, static_cast<long>(value));
    logResult(log, source, status, message);
}

void run() {
    auto& port = board::console();
    Logger log{port};
    FrameParser parser;
    port.setWakeup(wakeConsole, osThreadGetId());
    auto stream = port.startStream();
    if (stream != Status::Ok) logResult(log, "UART", stream, "stream init");
    startServices(log);
    for (;;) {
        osThreadFlagsWait(consoleEvent | serviceEvent, osFlagsWaitAny, FrameParser::gapMs);
        uint8_t byte{};
        uint32_t arrival{};
        for (;;) {
            auto status = port.pop(byte, arrival);
            if (status == Status::Busy) break;
            if (status != Status::Ok) {
                parser = {};
                logResult(log, "UART", status, "receive stream");
                if (status != Status::Overflow) {
                    port.stopStream();
                    const auto recovery = port.startStream();
                    if (recovery != Status::Ok) logResult(log, "UART", recovery, "stream recovery");
                }
                break;
            }
            Frame frame;
            status = parser.push(byte, arrival, frame);
            if (status == Status::Ok) exchange(frame, log);
            else if (status != Status::Busy) logResult(log, "FRAME", status, "receive");
        }
        if (parser.expire(board::millis()) == Status::Timeout)
            logResult(log, "FRAME", Status::Timeout, "partial frame");
        serviceEvents(log);
    }
}
}

extern "C" void Framework_Run() { fw::app::run(); }
