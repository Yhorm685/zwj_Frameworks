#include "BSP/spi.hpp"
#include "BSP/f103_transfer.hpp"
#include "spi.h"
#include <cstring>

extern "C" osSemaphoreId_t spiDoneHandle;

namespace fw::spi {
namespace {
std::atomic_flag gate = ATOMIC_FLAG_INIT;
detail::Operation operation;
detail::CompletionSignal completion{spiDoneHandle};
uint8_t buffer[128];

void notifyCompletion(SPI_HandleTypeDef* handle) {
    if (handle == &hspi1) completion.notify();
}

void select(bool active) { HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, active ? GPIO_PIN_RESET : GPIO_PIN_SET); }
void commandData(bool data) { HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, data ? GPIO_PIN_SET : GPIO_PIN_RESET); }

Status restore() {
    select(false);
    operation.active = false;
    if (hspi1.Instance != SPI1) return Status::NotInitialized;
    __HAL_SPI_DISABLE_IT(&hspi1, SPI_IT_TXE | SPI_IT_RXNE | SPI_IT_ERR);
    const auto stopped = HAL_SPI_DeInit(&hspi1);
    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();
    HAL_NVIC_ClearPendingIRQ(SPI1_IRQn);
    return stopped == HAL_OK ? detail::status(HAL_SPI_Init(&hspi1)) : detail::status(stopped);
}

Status transmit(bool data, const uint8_t* bytes, size_t size, Ticket* ticket, uint32_t ms) {
    if (!bytes || size == 0 || size > sizeof(buffer) || !detail::validTimeout(ms)) return Status::InvalidArgument;
    detail::Gate lock(gate);
    if (!lock || operation.active) return Status::Busy;
    if (hspi1.Instance != SPI1 || HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_RESET) return Status::NotInitialized;
    if (ticket && !completion.reset()) return Status::NotInitialized;
    commandData(data);
    select(true);
    if (!ticket) {
        const auto rc = HAL_SPI_Transmit(&hspi1, bytes, static_cast<uint16_t>(size), ms);
        select(false);
        if (rc == HAL_OK) return Status::Ok;
        const auto recovered = restore();
        return recovered == Status::Ok ? detail::status(rc) : recovered;
    }
    std::memcpy(buffer, bytes, size);
    operation.arm(*ticket, ms);
    const auto rc = HAL_SPI_Transmit_IT(&hspi1, buffer, static_cast<uint16_t>(size));
    if (rc != HAL_OK) {
        const auto recovered = restore();
        return recovered == Status::Ok ? detail::status(rc) : recovered;
    }
    return Status::Ok;
}
}

Status startDisplay(bool data, const uint8_t* bytes, size_t size, Ticket& ticket, uint32_t ms) {
    return transmit(data, bytes, size, &ticket, ms);
}

Status wait(Ticket ticket) {
    detail::Gate lock(gate);
    if (!lock) return Status::Busy;
    if (!operation.owns(ticket)) return Status::Cancelled;
    while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY) {
        if (operation.timedOut()) {
            const auto recovered = restore();
            return recovered == Status::Ok ? Status::Timeout : recovered;
        }
        completion.wait(operation.start, operation.duration);
    }
    operation.active = false;
    select(false);
    if (HAL_SPI_GetError(&hspi1) == HAL_SPI_ERROR_NONE) return Status::Ok;
    const auto recovered = restore();
    return recovered == Status::Ok ? Status::BusError : recovered;
}

Status writeDisplay(bool data, const uint8_t* bytes, size_t size, TransferMode mode, uint32_t ms) {
    if (mode == TransferMode::Blocking) return transmit(data, bytes, size, nullptr, ms);
    Ticket ticket;
    auto rc = startDisplay(data, bytes, size, ticket, ms);
    if (rc != Status::Ok) return rc;
    do {
        rc = wait(ticket);
        if (rc == Status::Busy) detail::nextTick();
    } while (rc == Status::Busy);
    return rc;
}

Status resetDisplay() {
    detail::Gate lock(gate);
    if (!lock || operation.active) return Status::Busy;
    if (hspi1.Instance != SPI1) return Status::NotInitialized;
    select(false);
    HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(10) + 1);
    HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(10) + 1);
    return Status::Ok;
}
Status cancel(Ticket ticket) {
    detail::Gate lock(gate);
    if (!lock) return Status::Busy;
    return operation.owns(ticket) ? restore() : Status::Cancelled;
}
Status recover() {
    detail::Gate lock(gate);
    return lock ? restore() : Status::Busy;
}
}

extern "C" {
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* handle) { fw::spi::notifyCompletion(handle); }
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* handle) { fw::spi::notifyCompletion(handle); }
}
