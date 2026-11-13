#pragma once

#include "Common/status.hpp"
#include <cstddef>

namespace fw {
class Logger {
public:
    template <typename Writer>
    explicit Logger(Writer& writer) : context_(&writer), sink_(
        [](void* context, const std::uint8_t* data, std::size_t size, std::uint32_t timeoutMs) {
            return static_cast<Writer*>(context)->write(data, size, timeoutMs);
        }) {}
    Status write(const char* module, Status event, const char* message, std::uint32_t timeoutMs);
private:
    void* context_;
    Status (*sink_)(void*, const std::uint8_t*, std::size_t, std::uint32_t);
};
}
