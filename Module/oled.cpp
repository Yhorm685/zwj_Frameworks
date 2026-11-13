#include "Module/oled.hpp"
#include "BSP/spi.hpp"
#include <array>
#include <atomic>
#include <algorithm>

namespace fw::oled {
namespace {
enum class Screen { Off, Pattern, Sample };
Screen current = Screen::Off;
std::atomic_flag gate = ATOMIC_FLAG_INIT;
using Page = std::array<uint8_t, 128>;

Status initialize(TransferMode mode) {
    auto rc = spi::resetDisplay();
    if (rc != Status::Ok) return rc;
    constexpr uint8_t init[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    return spi::writeDisplay(false, init, sizeof init, mode);
}

Status writePage(uint8_t page, const Page& data, TransferMode mode) {
    const uint8_t pos[]{static_cast<uint8_t>(0xB0 | page), 0x00, 0x10};
    const auto rc = spi::writeDisplay(false, pos, sizeof pos, mode);
    return rc == Status::Ok ? spi::writeDisplay(true, data.data(), data.size(), mode) : rc;
}

Status render(Screen target, int16_t ax, TransferMode mode) {
    if (target == Screen::Pattern || current == Screen::Off) {
        const auto rc = initialize(mode);
        if (rc != Status::Ok) return rc;
    }
    Page data{};
    for (uint8_t p = 0; p < 8; ++p) {
        if (target == Screen::Sample && current == Screen::Sample && p != 3) continue;
        data.fill(0);
        if (target == Screen::Pattern) {
            for (size_t x = 0; x < data.size(); ++x) {
                data[x] = ((x / 16 + p) % 2) ? 0xFF : 0x00;
            }
            // Diagonal through the checkerboard, then four direction marks and an unbroken border.
            for (size_t i = 0; i < 16; ++i) data[p * 16 + i] ^= 1U << (i / 2);
            if (p == 0 || p == 7) {
                const unsigned leftBars = p == 0 ? 1U : 4U;
                const unsigned rightBars = p == 0 ? 2U : 3U;
                for (unsigned i = 0; i < 12; ++i) {
                    data[2 + i] = i % 3 == 0 && i / 3 < leftBars ? 0x3C : 0;
                    data[125 - i] = i % 3 == 0 && i / 3 < rightBars ? 0x3C : 0;
                }
                for (auto& column : data) column |= p == 0 ? 0x01 : 0x80;
            }
            data.front() = data.back() = 0xFF;
        } else if (p == 3) {
            const int end = 64 + std::clamp(static_cast<int>(ax) / 512, -63, 63);
            for (int x = std::min(64, end); x <= std::max(64, end); ++x) data[x] = 0x3C;
            data[0] = data[64] = data[127] = 0xFF;
        }
        const auto rc = writePage(p, data, mode);
        if (rc != Status::Ok) return rc;
    }
    return Status::Ok;
}

Status display(Screen next, int16_t ax, TransferMode mode) {
    if (gate.test_and_set()) return Status::Busy;
    const auto rc = render(next, ax, mode);
    current = rc == Status::Ok ? next : Screen::Off;
    gate.clear();
    return rc;
}
}

Status showPattern(TransferMode mode) { return display(Screen::Pattern, 0, mode); }
Status showSample(int16_t ax, TransferMode mode) { return display(Screen::Sample, ax, mode); }
}
