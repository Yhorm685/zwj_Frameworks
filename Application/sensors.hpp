#pragma once
#include "Application/commands.hpp"
#include "Module/mpu6050.hpp"

namespace fw::app {
void startWorkers(Logger& log);
void drainWorkers(Logger& log);
void logWorkerResources(Logger& log);
bool sensorCommand(Frame frame, Logger& log);
bool latestSample(mpu6050::Sample& output);
}
