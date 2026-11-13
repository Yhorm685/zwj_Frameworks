#include "Common/logger.hpp"
#include <cstdio>

namespace fw {
Status Logger::write(const char* module, Status event, const char* message, std::uint32_t timeoutMs) {
    if (!module || !message || !*module) return Status::InvalidArgument;
    char line[128];
    const int count = std::snprintf(line, sizeof(line), "[%s][%s] %s\r\n", module, statusName(event), message);
    if (count < 0 || static_cast<std::size_t>(count) >= sizeof(line)) return Status::Overflow;
    return sink_(context_, reinterpret_cast<const std::uint8_t*>(line), static_cast<std::size_t>(count), timeoutMs);
}
}
