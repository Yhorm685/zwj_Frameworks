#include "Application/sensors.hpp"
#include "Application/rtos_messages.h"
#include "BSP/board_timer.hpp"
#include "BSP/i2c.hpp"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include <atomic>
#include <cstdio>
#include <type_traits>

extern "C" {
extern osThreadId_t commHandle, sensorHandle, monitorHandle;
extern osMessageQueueId_t requestQueueHandle, eventQueueHandle;
}

namespace fw::app {
namespace {
enum EventKind : uint8_t { Sample, Snapshot, Init, MissingAddress, Monitor };
using Event = FrameworkSensorEvent;
static_assert(sizeof(Frame) == 2 && sizeof(Frame) == sizeof(FrameworkRequest));
static_assert(std::is_trivially_copyable_v<Frame>);
constexpr uint32_t workerReady = 1U;
std::atomic<uint32_t> dropped{}, lateCycles{};
mpu6050::Sample lastSample{};
bool validSample{};
bool workersReady{};
Event lastMonitor{};
bool haveMonitorReport{};

void post(const Event& event) {
    if (osMessageQueuePut(eventQueueHandle, &event, 0, 0) != osOK) ++dropped;
    osThreadFlagsSet(commHandle, 2U);
}

void sampleTask() {
    auto mode = TransferMode::Interrupt;
    bool online = false;
    uint32_t deadline = osKernelGetTickCount();
    const uint32_t period = osKernelGetTickFreq() / 50;
    unsigned cycles{};
    bool first = true;
    while (true) {
        Frame request{};
        const bool commanded = osMessageQueueGet(requestQueueHandle, &request, nullptr, 0) == osOK;
        Event event{};
        const bool initRequested = commanded && request.command == 0x20;
        if (first || initRequested) {
            first = false;
            if (initRequested)
                mode = request.argument ? TransferMode::Interrupt : TransferMode::Blocking;
            auto status = i2c::recover();
            if (status == Status::Ok) status = mpu6050::initialize(mode);
            online = status == Status::Ok;
            if (initRequested || !online) {
                event.kind = EventKind::Init;
                event.result = static_cast<uint8_t>(status);
                post(event);
            }
        }
        if (commanded && request.command == 0x22) {
            uint8_t unused{};
            event.kind = EventKind::MissingAddress;
            event.result = static_cast<uint8_t>(i2c::read(0x67, 0x75, &unused, 1, mode));
            post(event);
        }
        const bool snapshot = commanded && request.command == 0x21;
        if (online || snapshot) {
            event.kind = snapshot ? EventKind::Snapshot : EventKind::Sample;
            const auto readMode = snapshot ? (request.argument ? TransferMode::Interrupt : TransferMode::Blocking) : mode;
            mpu6050::Sample value{};
            const auto result = online ? mpu6050::sample(value, readMode) : Status::NotInitialized;
            event.result = static_cast<uint8_t>(result);
            if (result != Status::Ok) online = false;
            event.ax = value.ax; event.ay = value.ay; event.az = value.az;
            event.temperature = value.temperature;
            event.gx = value.gx; event.gy = value.gy; event.gz = value.gz;
            if (snapshot || !online || initRequested || cycles % 50 == 0) post(event);
        }
        ++cycles;
        deadline += period;
        const auto now = osKernelGetTickCount();
        if (static_cast<int32_t>(deadline - now) <= 0) {
            ++lateCycles;
            deadline = now + period;
        }
        osDelayUntil(deadline);
    }
}

void monitorTask() {
    uint32_t deadline = osKernelGetTickCount();
    uint32_t lastTime = deadline;
    for (;;) {
        deadline += osKernelGetTickFreq();
        if (static_cast<int32_t>(deadline - osKernelGetTickCount()) <= 0)
            deadline = osKernelGetTickCount() + osKernelGetTickFreq();
        osDelayUntil(deadline);
        const uint32_t now = osKernelGetTickCount();
        Event report{};
        report.kind = EventKind::Monitor;
        report.elapsedMs = now - lastTime;  // Both board configurations use a 1 kHz RTOS tick.
        report.heap = xPortGetFreeHeapSize();
        report.commStack = osThreadGetStackSpace(commHandle);
        report.sensorStack = osThreadGetStackSpace(sensorHandle);
        report.monitorStack = osThreadGetStackSpace(osThreadGetId());
        post(report);
        lastTime = now;
    }
}
}

void startWorkers(Logger& log) {
    if (!commHandle || !sensorHandle || !monitorHandle || !requestQueueHandle || !eventQueueHandle) {
        logResult(log, "RTOS", Status::NotInitialized, "CubeMX task/queue allocation");
        return;
    }
    if (osMessageQueueGetMsgSize(requestQueueHandle) != sizeof(Frame) ||
        osMessageQueueGetMsgSize(eventQueueHandle) != sizeof(Event)) {
        logResult(log, "RTOS", Status::InvalidArgument, "CubeMX queue item size");
        return;
    }
    // The Comm task calls this after requesting periodic TIM2. Verify the interrupt really fires.
    auto& timer = board::clockTimer();
    const auto start = timer.ticks();
    for (unsigned n = 0; n < 10 && timer.ticks() == start; ++n) osDelay(1);
    if (timer.ticks() == start) {
        logResult(log, "RTOS", Status::Timeout, "TIM2 startup");
        return;
    }
    const auto first = osThreadFlagsSet(sensorHandle, workerReady);
    const auto second = osThreadFlagsSet(monitorHandle, workerReady);
    workersReady = ((first | second) & osFlagsError) == 0;
    if (!workersReady) logResult(log, "RTOS", Status::NotInitialized, "Sensor/Monitor startup");
}

bool latestSample(mpu6050::Sample& output) {
    if (!validSample) return false;
    output = lastSample;
    return true;
}

bool sensorCommand(Frame frame, Logger& log) {
    if (frame.command < 0x20 || frame.command > 0x22) return false;
    Status result = Status::NotInitialized;
    if (frame.argument > 1) result = Status::InvalidArgument;
    else if (workersReady)
        result = osMessageQueuePut(requestQueueHandle, &frame, 0, 0) == osOK ? Status::Ok : Status::Overflow;
    if (result != Status::Ok) logResult(log, "SENSOR", result, "command enqueue");
    return true;
}

void logWorkerResources(Logger& log) {
    if (!workersReady) {
        logResult(log, "MON", Status::NotInitialized, "workers");
        return;
    }
    if (!haveMonitorReport) {
        logResult(log, "MON", Status::Busy, "first report pending");
        return;
    }
    char line[112];
    std::snprintf(line, sizeof(line), "dt_ms=%lu heap=%lu stacks=%lu/%lu/%lu lost=%lu late=%lu",
        (unsigned long)lastMonitor.elapsedMs, (unsigned long)lastMonitor.heap,
        (unsigned long)lastMonitor.commStack, (unsigned long)lastMonitor.sensorStack,
        (unsigned long)lastMonitor.monitorStack, (unsigned long)dropped.load(), (unsigned long)lateCycles.load());
    logResult(log, "MON", Status::Ok, line);
}

void drainWorkers(Logger& log) {
    Event e{};
    while (workersReady && osMessageQueueGet(eventQueueHandle, &e, nullptr, 0) == osOK) {
        char line[88];
        const auto result = static_cast<Status>(e.result);
        switch (e.kind) {
        case EventKind::Monitor:
            lastMonitor = e;
            haveMonitorReport = true;
            break;
        case EventKind::Init:
            validSample = false;
            logResult(log, "MPU", result, "init");
            break;
        case EventKind::MissingAddress:
            logResult(log, "I2C", result, "probe 0x67");
            break;
        case EventKind::Sample:
        case EventKind::Snapshot:
            validSample = result == Status::Ok;
            if (!validSample) {
                logResult(log, "MPU", result, "sample");
                break;
            }
            lastSample = {e.ax, e.ay, e.az, e.temperature, e.gx, e.gy, e.gz};
            if (e.kind == EventKind::Sample) break;
            std::snprintf(line, sizeof(line), "acc=%d,%d,%d gyro=%d,%d,%d",
                e.ax, e.ay, e.az, e.gx, e.gy, e.gz);
            logResult(log, "MPU", result, line);
            break;
        }
    }
}
}

extern "C" void StartSensor(void*) {
    if (osThreadFlagsWait(fw::app::workerReady, osFlagsWaitAny, osWaitForever) == fw::app::workerReady)
        fw::app::sampleTask();
    osThreadExit();
}

extern "C" void StartMonitor(void*) {
    if (osThreadFlagsWait(fw::app::workerReady, osFlagsWaitAny, osWaitForever) == fw::app::workerReady)
        fw::app::monitorTask();
    osThreadExit();
}
