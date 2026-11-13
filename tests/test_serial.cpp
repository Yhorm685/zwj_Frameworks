#include "BSP/uart.hpp"
#include "Common/logger.hpp"
#include "Module/frame.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
unsigned checks{}, wakes{}, rxAborts{}, rxArms{}, txAborts{};
HAL_StatusTypeDef halReply = HAL_OK;
std::uint8_t* rxData{};
const std::uint8_t* txData{};
std::string logged;
void check(bool ok, int line) {
    ++checks;
    if (!ok) { std::cerr << "serial check failed at line " << line << '\n'; std::exit(1); }
}
#define CHECK(test) check(static_cast<bool>(test), __LINE__)
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*, const std::uint8_t* data,
                                   std::uint16_t size, std::uint32_t) {
    logged.assign(reinterpret_cast<const char*>(data), size);
    return halReply;
}
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef*, std::uint8_t*, std::uint16_t, std::uint32_t) {
    return halReply;
}
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef*, const std::uint8_t* data, std::uint16_t) {
    txData = data;
    return halReply;
}
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef*, std::uint8_t* data, std::uint16_t) {
    ++rxArms; rxData = data;
    return halReply;
}
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef*) { ++rxAborts; return halReply; }
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef*) { ++txAborts; return halReply; }

int main() {
    using fw::Status;
    fw::FrameParser parser;
    fw::Frame frame{};
    CHECK(parser.push(0, 0, frame) == Status::Busy);
    auto packet = [&](std::uint8_t cmd, std::uint8_t arg, std::uint32_t now) {
        CHECK(parser.push(0xA5, now, frame) == Status::Busy);
        CHECK(parser.push(cmd, now, frame) == Status::Busy);
        CHECK(parser.push(arg, now, frame) == Status::Busy);
        return parser.push(static_cast<std::uint8_t>(0xA5 + cmd + arg), now, frame);
    };
    CHECK(packet(0xA5, 0xA5, 1) == Status::Ok && frame.command == 0xA5 && frame.argument == 0xA5);
    CHECK(packet(0, 0, 1) == Status::Ok);
    parser.push(0xA5, UINT32_MAX - 2, frame);
    CHECK(parser.expire(16) == Status::Busy);
    CHECK(parser.push(0xA5, 17, frame) == Status::Timeout);
    parser.push(2, 17, frame); parser.push(3, 17, frame);
    CHECK(parser.push(0xA5, 17, frame) == Status::ChecksumError);
    parser.push(3, 17, frame); parser.push(4, 17, frame);
    CHECK(parser.push(0xAC, 17, frame) == Status::Ok && frame.command == 3 && frame.argument == 4);
    UART_HandleTypeDef handle{};
    fw::Uart uart{handle};
    std::uint8_t byte = 1;
    std::uint32_t time{};
    CHECK(uart.read(&byte, 1, 10) == Status::NotInitialized);
    handle = {&handle, HAL_UART_STATE_READY, HAL_UART_STATE_READY, 0};
    uart.setWakeup([](void*) { ++wakes; }, nullptr);
    CHECK(uart.startRead(0, 10) == Status::InvalidArgument);
    CHECK(uart.write(nullptr, 1, 10) == Status::InvalidArgument);
    CHECK(uart.startRead(129, 10) == Status::InvalidArgument);
    CHECK(uart.startRead(1, HAL_MAX_DELAY) == Status::InvalidArgument);
    CHECK(uart.startWrite(&byte, 1, 10) == Status::Ok);
    byte = 8;
    CHECK(*txData == 1);
    CHECK(uart.startWrite(&byte, 1, 10) == Status::Busy);
    CHECK(uart.pollWrite() == Status::Busy);
    CHECK(uart.startRead(1, 10) == Status::Ok);
    CHECK(uart.startStream() == Status::Busy);
    CHECK(uart.read(&byte, 1, 10) == Status::Busy);
    *rxData = 42;
    HAL_UART_RxCpltCallback(&handle);
    HAL_UART_TxCpltCallback(&handle);
    CHECK(wakes == 2);
    CHECK(uart.pollRead(nullptr, 1) == Status::InvalidArgument);
    CHECK(uart.pollRead(&byte, 1) == Status::Ok && byte == 42);
    CHECK(uart.pollRead(&byte, 1) == Status::NotInitialized);
    CHECK(uart.pollWrite() == Status::Ok);
    testTick = UINT32_MAX - 2;
    CHECK(uart.startRead(1, 10) == Status::Ok);
    testTick = 7;
    CHECK(uart.pollRead(&byte, 1) == Status::Timeout && rxAborts == 1);
    CHECK(uart.startRead(1, 10) == Status::Ok);
    HAL_UART_ErrorCallback(&handle);
    CHECK(uart.pollRead(&byte, 1) == Status::BusError);
    CHECK(uart.startRead(1, 10) == Status::Ok);
    CHECK(uart.cancelRead() == Status::Cancelled);
    CHECK(uart.startWrite(&byte, 1, 3) == Status::Ok);
    testTick += 4;
    HAL_UART_TxCpltCallback(&handle);
    CHECK(uart.pollWrite() == Status::Timeout && txAborts == 1);
    halReply = HAL_BUSY;
    CHECK(uart.startRead(1, 10) == Status::Busy);
    halReply = HAL_TIMEOUT;
    CHECK(uart.read(&byte, 1, 10) == Status::Timeout);
    handle.ErrorCode = 8;
    CHECK(uart.read(&byte, 1, 10) == Status::BusError);
    handle.ErrorCode = 0;
    halReply = HAL_OK;
    fw::Logger log{uart};
    CHECK(log.write("UART", Status::Timeout, "read", 100) == Status::Ok);
    CHECK(logged == "[UART][Timeout] read\r\n");
    CHECK(log.write("", Status::Ok, "x", 100) == Status::InvalidArgument);
    CHECK(log.write("x", Status::Ok, std::string(140, 'a').c_str(), 100) == Status::Overflow);
    CHECK(uart.startStream() == Status::Ok);
    CHECK(uart.startRead(1, 10) == Status::Busy);
    CHECK(uart.pop(byte, time) == Status::Busy);
    const auto beforeBurst = rxArms;
    for (unsigned i = 0; i < 8; ++i) {
        *rxData = static_cast<std::uint8_t>(i);
        testTick = i * 3;
        HAL_UART_RxCpltCallback(&handle);
    }
    CHECK(rxArms == beforeBurst + 8);
    for (unsigned i = 0; i < 8; ++i) {
        CHECK(uart.pop(byte, time) == Status::Ok && byte == i && time == i * 3);
    }
    CHECK(uart.pop(byte, time) == Status::Busy);
    for (unsigned i = 0; i < 64; ++i) {
        *rxData = static_cast<std::uint8_t>(i);
        HAL_UART_RxCpltCallback(&handle);
    }
    CHECK(uart.pop(byte, time) == Status::Overflow && uart.streamOverruns() == 1);
    CHECK(uart.pop(byte, time) == Status::Ok && byte == 63);
    const auto beforeError = rxArms;
    handle.ErrorCode = 8;
    HAL_UART_RxCpltCallback(&handle);
    CHECK(rxArms == beforeError);
    HAL_UART_ErrorCallback(&handle);
    CHECK(uart.pop(byte, time) == Status::BusError && rxArms == beforeError + 1 && !handle.ErrorCode);
    CHECK(uart.pop(byte, time) == Status::Busy);
    *rxData = 87; HAL_UART_RxCpltCallback(&handle);
    CHECK(uart.pop(byte, time) == Status::Ok && byte == 87);
    CHECK(uart.stopStream() == Status::Ok);
    CHECK(uart.pop(byte, time) == Status::NotInitialized);
    CHECK(uart.startRead(1, 5) == Status::Ok && uart.cancelRead() == Status::Cancelled);
    CHECK(testMask == 0);
    std::cout << "PASS serial and frame: " << checks << " checks (mock HAL)\n";
}
