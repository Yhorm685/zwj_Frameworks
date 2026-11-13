#pragma once
#include "BSP/timer.hpp"
#ifdef FRAMEWORK_F103
#include "Module/sg90.hpp"
#endif

namespace fw::board {
Timer& clockTimer();
#ifdef FRAMEWORK_F103
Sg90& servo(bool second);
#endif
}
