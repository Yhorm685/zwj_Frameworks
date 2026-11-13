#include "BSP/can.hpp"
#include "BSP/i2c.hpp"
#include "BSP/spi.hpp"
#include "Module/mpu6050.hpp"
#include "Module/oled.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>

namespace {
using fw::Status;
using fw::TransferMode;
std::array<uint8_t, 256> registers{};
std::array<std::array<uint8_t, 128>, 8> pixels{};
uint8_t selectedPage{};
unsigned pages{}, checks{};
bool connected = true;
void check(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", name); std::exit(1); }
}
}

namespace fw::i2c {
Status read(uint8_t addr, uint8_t reg, uint8_t* out, size_t n, TransferMode, uint32_t) {
    if (!connected || addr != 0x69) return Status::BusError;
    std::copy_n(registers.begin() + reg, n, out);
    return Status::Ok;
}
Status write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t n, TransferMode, uint32_t) {
    if (!connected || addr != 0x69) return Status::BusError;
    std::copy_n(data, n, registers.begin() + reg);
    return Status::Ok;
}
}

namespace fw::spi {
Status resetDisplay() { return Status::Ok; }
Status writeDisplay(bool data, const uint8_t* bytes, size_t n, TransferMode, uint32_t) {
    if (!connected) return Status::BusError;
    if (!data && n == 3) selectedPage = bytes[0] & 7U;
    if (data) {
        if (n != pixels.front().size()) return Status::InvalidArgument;
        std::copy_n(bytes, n, pixels[selectedPage].begin());
        ++pages;
    }
    return Status::Ok;
}
}

int main() {
    using namespace fw;
    const auto mode = TransferMode::Interrupt;
    mpu6050::Sample sample{};
    check(mpu6050::sample(sample, mode) == Status::NotInitialized, "read before init");
    check(mpu6050::initialize(mode) == Status::DeviceMismatch, "incorrect WHO_AM_I");
    registers[0x75] = 0x68;
    check(mpu6050::initialize(mode) == Status::Ok, "AD0 high with correct ID");
    registers[0x3B] = 0xC0;
    registers[0x43] = 0xFF;
    registers[0x44] = 0x7D;
    check(mpu6050::sample(sample, mode) == Status::Ok, "burst read");
    check(sample.ax == -16384 && sample.gx == -131, "signed big endian");
    check(mpu6050::accelMilliG(sample.ax) == -1000, "acceleration scale");
    check(mpu6050::gyroMilliDps(sample.gx) == -1000, "gyro scale");
    connected = false;
    check(mpu6050::sample(sample, mode) == Status::BusError, "read failure");
    check(oled::showPattern(mode) == Status::BusError, "display failure");
    connected = true;
    check(oled::showPattern(mode) == Status::Ok && pages == 8, "draw complete display");
    for (const auto& page : pixels) check(page.front() == 0xFF && page.back() == 0xFF, "side borders");
    for (unsigned x = 0; x < 128; ++x)
        check((pixels[0][x] & 1U) && (pixels[7][x] & 0x80U), "horizontal borders");
    check(oled::showSample(16384, mode) == Status::Ok, "positive sample");
    check(pixels[3][90] && !pixels[3][40], "positive bar points right");
    const auto before = pages;
    check(oled::showSample(-16384, mode) == Status::Ok && pages == before + 1, "one-page refresh");
    check(!pixels[3][90] && pixels[3][40], "previous bar cleared");
    check(can::validFrame({0x7FF, 8, {}}), "standard data frame");
    check(!can::validFrame({0x800, 8, {}}), "invalid identifier");
    check(!can::validFrame({0, 9, {}}), "invalid DLC");
    std::printf("PASS devices: %u checks (mock buses)\n", checks);
}
