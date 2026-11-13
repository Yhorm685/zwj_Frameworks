#include "BSP/timer.hpp"
#include "Module/sg90.hpp"
#include <cstdlib>
#include <iostream>

extern "C" void Framework_TimerElapsed(TIM_HandleTypeDef*);

namespace {
unsigned checks{}, wakes{};
HAL_StatusTypeDef halReply = HAL_OK;
void countWake(void*) { ++wakes; }
void check(bool ok, int line) {
    ++checks;
    if (!ok) { std::cerr << "timer check failed at line " << line << '\n'; std::exit(1); }
}
#define CHECK(test) check(static_cast<bool>(test), __LINE__)
}
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef* h) {
    if (halReply == HAL_OK) h->State = HAL_TIM_STATE_BUSY;
    return halReply;
}
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef* h) {
    if (halReply == HAL_OK) h->State = HAL_TIM_STATE_READY;
    return halReply;
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef* h) {
    if (halReply == HAL_OK) h->interrupts = TIM_IT_UPDATE;
    return HAL_TIM_Base_Start(h);
}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef* h) {
    if (halReply == HAL_OK) h->interrupts = 0;
    return HAL_TIM_Base_Stop(h);
}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*, std::uint32_t) { return halReply; }
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef*, std::uint32_t) { return halReply; }

int main() {
    using fw::Status;
    TIM_HandleTypeDef h2{}, h3{};
    fw::Timer base{h2};
    CHECK(base.waitUpdate(0) == Status::InvalidArgument);
    CHECK(base.startPeriodic() == Status::NotInitialized);
    h2 = {&h2, HAL_TIM_STATE_READY, 0, 999};
    h3 = {&h3, HAL_TIM_STATE_READY, 0, 19999};
    testTick = UINT32_MAX - 2;
    CHECK(base.waitUpdate(5) == Status::Timeout);
    testUpdateAfter = 2;
    CHECK(base.waitUpdate(5) == Status::Ok && h2.State == HAL_TIM_STATE_READY);
    CHECK(base.startPeriodic(countWake) == Status::Ok && h2.interrupts == TIM_IT_UPDATE);
    CHECK(base.startPeriodic() == Status::Busy);
    CHECK(base.waitUpdate(10) == Status::Busy);
    Framework_TimerElapsed(&h2); Framework_TimerElapsed(&h3); Framework_TimerElapsed(&h2);
    CHECK(base.ticks() == 2 && wakes == 2);
    CHECK(base.stopPeriodic() == Status::Ok && !h2.interrupts);
    Framework_TimerElapsed(&h2);
    CHECK(base.ticks() == 2 && wakes == 2);
    halReply = HAL_ERROR;
    CHECK(base.startPeriodic() == Status::BusError);
    halReply = HAL_OK;
    CHECK(base.startPeriodic() == Status::Ok && base.ticks() == 0);
    fw::PwmChannel yaw{h3, TIM_CHANNEL_3}, pitch{h3, TIM_CHANNEL_4};
    fw::Sg90 a{yaw}, b{pitch}, bad{yaw, 2000, 1000};
    CHECK(bad.start() == Status::InvalidArgument);
    CHECK(a.setAngle(90) == Status::NotInitialized);
    CHECK(a.start() == Status::Ok && h3.compare[2] == 1500);
    CHECK(a.start() == Status::Busy);
    CHECK(b.start() == Status::Ok && h3.compare[3] == 1500);
    CHECK(a.setAngle(-1) == Status::InvalidArgument && b.setAngle(181) == Status::InvalidArgument);
    CHECK(a.setAngle(0) == Status::Ok && h3.compare[2] == 1000);
    CHECK(a.setAngle(180) == Status::Ok && h3.compare[2] == 2000);
    CHECK(a.setAngle(89) == Status::Ok && h3.compare[2] == 1494);
    CHECK(yaw.setPulseUs(22000) == Status::InvalidArgument);
    CHECK(b.setAngleBlocking(45, 0) == Status::InvalidArgument);
    testUpdateAfter = 2;
    CHECK(b.setAngleBlocking(45, 25) == Status::Ok && h3.compare[3] == 1250);
    CHECK(b.setAngleBlocking(45, 3) == Status::Timeout);
    CHECK(b.startAngle(60, 25, countWake) == Status::Ok && h3.interrupts == TIM_IT_UPDATE);
    CHECK(b.setAngle(90) == Status::Busy && a.setAngleBlocking(45, 25) == Status::Busy);
    CHECK(a.startAngle(45, 25) == Status::Busy);
    CHECK(b.pollAngle() == Status::Busy);
    Framework_TimerElapsed(&h2);
    CHECK(b.pollAngle() == Status::Busy && wakes == 2);
    testTick += 20; Framework_TimerElapsed(&h3); testTick += 100;
    CHECK(b.pollAngle() == Status::Ok && !h3.interrupts && wakes == 3);
    CHECK(b.pollAngle() == Status::NotInitialized);
    CHECK(b.startAngle(30, 10, countWake) == Status::Ok);
    testTick += 10; Framework_TimerElapsed(&h3);
    CHECK(b.pollAngle() == Status::Timeout && !h3.interrupts);
    CHECK(b.startAngle(30, 10) == Status::Ok);
    testTick += 10;
    CHECK(b.pollAngle() == Status::Timeout);
    CHECK(b.startAngle(30, 10) == Status::Ok);
    CHECK(b.stop() == Status::Ok && !h3.interrupts);
    CHECK(b.pollAngle() == Status::NotInitialized && b.setAngle(90) == Status::NotInitialized);
    CHECK(a.startAngle(90, 10) == Status::Ok);
    CHECK(a.stop() == Status::Ok);
    CHECK(base.stopPeriodic() == Status::Ok);
    CHECK(testMask == 0);
    std::cout << "PASS timer and servo: " << checks << " checks (mock HAL)\n";
}
