#include "Application/commands.hpp"
#include "BSP/board_timer.hpp"
#include "cmsis_os2.h"
#ifdef FRAMEWORK_F103
#include "Application/sensors.hpp"
#include "Application/display.hpp"
#endif

namespace fw::app {
void startServices(Logger& log) {
    auto& clock = board::clockTimer();
    logResult(log, "TIM2", clock.waitUpdate(10), "polled update");
    logResult(log, "TIM2", clock.startPeriodic(), "1 ms interrupt");
#ifdef FRAMEWORK_F103
    startWorkers(log);
#endif
}
void serviceEvents(Logger& log) {
#ifdef FRAMEWORK_F103
    drainWorkers(log);
#else
    (void)log;
#endif
}

bool boardCommand(Frame frame, Logger& log) {
    if (frame.command == 0x10) {
        logValue(log, "TIM2", Status::Ok, "ticks", board::clockTimer().ticks());
        return true;
    }
#ifdef FRAMEWORK_F103
    if (displayCommand(frame, log)) return true;
    if (sensorCommand(frame, log)) return true;
    if (frame.command >= 0x11 && frame.command <= 0x14) {
        const bool second = frame.command == 0x12 || frame.command == 0x14;
        auto& axis = board::servo(second);
        static bool active[2]{};
        Status result = Status::Ok;
        if (frame.argument == 255) {
            result = axis.stop();
            if (result == Status::Ok) active[second] = false;
        } else if (frame.argument > 180) {
            result = Status::InvalidArgument;
        } else {
            if (!active[second]) {
                result = axis.start();
                active[second] = result == Status::Ok;
            }
            if (result == Status::Ok) {
                if (frame.command < 0x13) result = axis.setAngleBlocking(frame.argument, 60);
                else {
                    result = axis.startAngle(frame.argument, 60);
                    if (result == Status::Ok) {
                        while ((result = axis.pollAngle()) == Status::Busy) osDelay(1);
                    }
                }
            }
        }
        logValue(log, second ? "PITCH" : "YAW", result, "arg", frame.argument);
        return true;
    }
#endif
    return false;
}
}


