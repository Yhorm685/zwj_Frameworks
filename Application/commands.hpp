#pragma once

#include "Common/logger.hpp"
#include "Module/frame.hpp"

namespace fw::app {
// Called by the single owner of USART1. Workers send results to this task.
void startServices(Logger& log);
void serviceEvents(Logger& log);
bool boardCommand(Frame frame, Logger& log);
void logResult(Logger& log, const char* source, Status status, const char* message);
void logValue(Logger& log, const char* source, Status status, const char* label, int32_t value);
}
