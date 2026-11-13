#pragma once
#include "test_hal.hpp"
#include <array>

enum { HAL_TIM_STATE_RESET, HAL_TIM_STATE_READY, HAL_TIM_STATE_BUSY };
constexpr unsigned TIM_FLAG_UPDATE = 1, TIM_IT_UPDATE = 1;
constexpr unsigned TIM_CHANNEL_1 = 0, TIM_CHANNEL_2 = 4, TIM_CHANNEL_3 = 8, TIM_CHANNEL_4 = 12;
struct TIM_HandleTypeDef {
    void* Instance{};
    int State{};
    std::uint32_t counter{}, arr{}, flag{}, interrupts{};
    std::array<std::uint32_t, 4> compare{};
};
inline std::uint32_t testUpdateAfter{};
inline bool testFlag(TIM_HandleTypeDef* h, unsigned f) {
    ++testTick;
    if (testUpdateAfter && --testUpdateAfter == 0) h->flag |= f;
    return (h->flag & f) != 0;
}
#define __HAL_TIM_GET_FLAG(h, f) testFlag(h, f)
#define __HAL_TIM_CLEAR_FLAG(h, f) ((h)->flag &= ~(f))
#define __HAL_TIM_SET_COUNTER(h, c) ((h)->counter = (c))
#define __HAL_TIM_GET_AUTORELOAD(h) ((h)->arr)
#define __HAL_TIM_SET_COMPARE(h, ch, c) ((h)->compare[(ch) / 4] = (c))
#define __HAL_TIM_ENABLE_IT(h, i) ((h)->interrupts |= (i))
#define __HAL_TIM_DISABLE_IT(h, i) ((h)->interrupts &= ~(i))
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*, std::uint32_t);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef*, std::uint32_t);
