#pragma once
#include <stdint.h>

typedef struct {
    uint8_t command, argument;
} FrameworkRequest;

typedef struct {
    uint8_t kind, result;
    int16_t ax, ay, az, temperature, gx, gy, gz;
    uint32_t ticks, elapsedMs, heap, sensorStack, monitorStack, late;
} FrameworkSensorEvent;
