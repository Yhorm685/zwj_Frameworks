#include "BSP/board_timer.hpp"

namespace fw::board {
Timer& clockTimer() {
    static Timer timer{htim2};
    return timer;
}
#ifdef FRAMEWORK_F103
Sg90& servo(bool second) {
    static PwmChannel left{htim3, TIM_CHANNEL_3};
    static PwmChannel right{htim3, TIM_CHANNEL_4};
    static Sg90 yaw{left};
    static Sg90 pitch{right};
    return second ? pitch : yaw;
}
#endif
}
