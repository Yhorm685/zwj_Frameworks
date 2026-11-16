#include "Application/commands.hpp"
#include "BSP/board.hpp"
#include "cmsis_os2.h"
#include <cstdio>

namespace fw::app {
namespace {
constexpr uint32_t consoleEvent = 1U;
constexpr uint32_t serviceEvent = 2U;
// Comm owns both the log sink and the counters for reported events.
uint32_t busErrors{}, timeouts{}, checksumErrors{}, logFailures{};

void wakeConsole(void* task) {
    osThreadFlagsSet(task, consoleEvent);
}

void reportByte(Logger& log, Status status, uint8_t byte) {
    logValue(log, "UART", status, "byte", byte);
}

void exchange(Frame frame, Uart& port, Logger& log) {
    switch (frame.command) {
    case 0x01:
        logValue(log, "ECHO", Status::Ok, "arg", frame.argument);
        break;
    case 0x02: {
        const uint8_t line[] = {'I','T',' ','T','X','\r','\n'};
        Status result = port.startWrite(line, sizeof(line), 50);
        if (result == Status::Ok) {
            const auto again = port.startWrite(line, sizeof(line), 50);
            do {
                result = port.pollWrite();
                if (result == Status::Busy) osThreadFlagsWait(consoleEvent, osFlagsWaitAny, 1);
            } while (result == Status::Busy);
            logResult(log, "UART", again, "second start");
        }
        logResult(log, "UART", result, "interrupt TX final");
        break;
    }
    case 0x03:
    case 0x04: {
        auto result = port.stopStream();
        uint8_t byte{};
        if (result == Status::Ok) {
            logResult(log, "UART", Status::Ok, "send one byte within 500 ms");
            if (frame.command == 0x03) {
                result = port.read(&byte, 1, 500);
            } else {
                result = port.startRead(1, 500);
                if (result == Status::Ok) {
                    do {
                        result = port.pollRead(&byte, 1);
                        if (result == Status::Busy) osThreadFlagsWait(consoleEvent, osFlagsWaitAny, 1);
                    } while (result == Status::Busy);
                }
            }
        }
        const auto restart = port.startStream();
        reportByte(log, result, byte);
        if (restart != Status::Ok) logResult(log, "UART", restart, "stream restart");
        break;
    }
    case 0x05:
        logValue(log, "COMM", Status::Ok, "stack_free_B", osThreadGetStackSpace(osThreadGetId()));
        logValue(log, "COMM", Status::Ok, "log_errors", logFailures);
        break;
    default:
        if (!boardCommand(frame, log)) logResult(log, "CMD", Status::InvalidArgument, "unknown command");
    }
}
}

void logResult(Logger& log, const char* source, Status status, const char* message) {
    switch (status) {
    case Status::BusError: ++busErrors; break;
    case Status::Timeout: ++timeouts; break;
    case Status::ChecksumError: ++checksumErrors; break;
    default: break;
    }
    if (log.write(source, status, message, 50) != Status::Ok) ++logFailures;
}

#ifdef FRAMEWORK_F103
void logCommunicationErrors(Logger& log) {
    char message[80];
    std::snprintf(message, sizeof(message), "bus=%lu timeout=%lu checksum=%lu log_write=%lu",
        static_cast<unsigned long>(busErrors), static_cast<unsigned long>(timeouts),
        static_cast<unsigned long>(checksumErrors), static_cast<unsigned long>(logFailures));
    logResult(log, "MON", Status::Ok, message);
}
#endif

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
    logResult(log, board::name(), Status::Ok, "framework ready");
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
            if (status == Status::Ok) exchange(frame, port, log);
            else if (status != Status::Busy) logResult(log, "FRAME", status, "receive");
        }
        if (parser.expire(board::millis()) == Status::Timeout)
            logResult(log, "FRAME", Status::Timeout, "partial frame");
        serviceEvents(log);
    }
}
}

extern "C" void Framework_Run() { fw::app::run(); }
