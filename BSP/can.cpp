#include "BSP/can.hpp"
#include "BSP/f103_transfer.hpp"
#include "can.h"
#include <array>

namespace fw::can {
namespace {
std::atomic_flag gate = ATOMIC_FLAG_INIT;
detail::Operation tx;
uint32_t mailbox{};
bool online{}, internalLoopback{};
TransferMode transferMode = TransferMode::Blocking;
std::atomic<Status> irqTx{Status::Busy}, rxFault{Status::Ok};
std::array<Frame, 8> ring;
std::atomic<unsigned> head{}, tail{};
constexpr uint32_t txErrors = HAL_CAN_ERROR_TX_ALST0 | HAL_CAN_ERROR_TX_TERR0 |
    HAL_CAN_ERROR_TX_ALST1 | HAL_CAN_ERROR_TX_TERR1 | HAL_CAN_ERROR_TX_ALST2 | HAL_CAN_ERROR_TX_TERR2;

Status setup(bool loopback, TransferMode mode) {
    online = false;
    tx.active = false;
    if (hcan.Instance != CAN1) return Status::NotInitialized;
    if (hcan.State != HAL_CAN_STATE_RESET) {
        hcan.Instance->IER = 0;
        const auto rc = HAL_CAN_DeInit(&hcan);
        if (rc != HAL_OK) return detail::status(rc);
    }
    __HAL_RCC_CAN1_FORCE_RESET();
    __HAL_RCC_CAN1_RELEASE_RESET();
    HAL_NVIC_ClearPendingIRQ(USB_HP_CAN1_TX_IRQn);
    HAL_NVIC_ClearPendingIRQ(USB_LP_CAN1_RX0_IRQn);
    HAL_NVIC_ClearPendingIRQ(CAN1_SCE_IRQn);
    head.store(0);
    tail.store(0);
    rxFault.store(Status::Ok);
    internalLoopback = loopback;
    transferMode = mode;
    hcan.Init.Mode = loopback ? CAN_MODE_SILENT_LOOPBACK : CAN_MODE_NORMAL;
    auto rc = HAL_CAN_Init(&hcan);
    if (rc != HAL_OK) return detail::status(rc);
    CAN_FilterTypeDef filter{};
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterMaskIdLow = CAN_RI0R_IDE | CAN_RI0R_RTR;  // Standard data frames only.
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    rc = HAL_CAN_ConfigFilter(&hcan, &filter);
    if (rc == HAL_OK) rc = HAL_CAN_Start(&hcan);
    if (rc == HAL_OK && mode == TransferMode::Interrupt) {
        rc = HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_RX_FIFO0_MSG_PENDING |
            CAN_IT_RX_FIFO0_OVERRUN | CAN_IT_BUSOFF | CAN_IT_ERROR);
    }
    online = rc == HAL_OK;
    return detail::status(rc);
}

Status readFifo(Frame& output) {
    CAN_RxHeaderTypeDef header{};
    Frame candidate{};
    const auto rc = HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &header, candidate.data);
    if (rc != HAL_OK) return detail::status(rc);
    if (header.IDE != CAN_ID_STD || header.RTR != CAN_RTR_DATA || header.DLC > 8U)
        return Status::BusError;
    candidate.id = static_cast<uint16_t>(header.StdId);
    candidate.length = static_cast<uint8_t>(header.DLC);
    output = candidate;
    return Status::Ok;
}

Status txResult() {
    if (transferMode == TransferMode::Interrupt) return irqTx.load();
    const unsigned shift = mailbox == CAN_TX_MAILBOX0 ? 0U : mailbox == CAN_TX_MAILBOX1 ? 8U : 16U;
    const uint32_t flags = hcan.Instance->TSR >> shift;
    if ((flags & CAN_TSR_RQCP0) == 0U) return Status::Busy;
    hcan.Instance->TSR = CAN_TSR_RQCP0 << shift;
    return (flags & CAN_TSR_TXOK0) ? Status::Ok : Status::BusError;
}
}

Status configure(bool loopback, TransferMode mode) {
    detail::Gate lock(gate);
    if (!lock || tx.active) return Status::Busy;
    return setup(loopback, mode);
}

Status startSend(const Frame& f, Ticket& ticket, uint32_t ms) {
    if (!validFrame(f) || !detail::validTimeout(ms)) return Status::InvalidArgument;
    detail::Gate lock(gate);
    if (!lock || tx.active) return Status::Busy;
    if (!online) return Status::NotInitialized;
    if (hcan.Instance->ESR & CAN_ESR_BOFF) return Status::BusError;
    if (!HAL_CAN_GetTxMailboxesFreeLevel(&hcan)) return Status::Busy;
    CAN_TxHeaderTypeDef header{};
    header.StdId = f.id;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = f.length;
    irqTx.store(Status::Busy);
    // We own all three mailboxes and allow one outstanding frame at a time.
    hcan.Instance->TSR = CAN_TSR_RQCP0 | CAN_TSR_RQCP1 | CAN_TSR_RQCP2;
    tx.arm(ticket, ms);
    const auto rc = HAL_CAN_AddTxMessage(&hcan, &header, f.data, &mailbox);
    if (rc != HAL_OK) tx.active = false;
    return detail::status(rc);
}

Status waitSent(Ticket ticket) {
    detail::Gate lock(gate);
    if (!lock) return Status::Busy;
    if (!tx.owns(ticket)) return Status::Cancelled;
    for (;;) {
        const auto rc = txResult();
        if (rc != Status::Busy) { tx.active = false; return rc; }
        const bool busOff = (hcan.Instance->ESR & CAN_ESR_BOFF) != 0;
        if (busOff || tx.timedOut()) {
            // Reset also stops a pending mailbox even when the bus never returns idle.
            const auto recovered = setup(internalLoopback, transferMode);
            if (recovered != Status::Ok) return recovered;
            return busOff ? Status::BusError : Status::Timeout;
        }
        detail::nextTick();
    }
}

Status send(const Frame& frame, uint32_t ms) {
    Ticket ticket;
    auto rc = startSend(frame, ticket, ms);
    if (rc != Status::Ok) return rc;
    do {
        rc = waitSent(ticket);
        if (rc == Status::Busy) detail::nextTick();
    } while (rc == Status::Busy);
    return rc;
}

Status receive(Frame& output, uint32_t ms) {
    if (ms > INT32_MAX) return Status::InvalidArgument;
    const uint32_t start = HAL_GetTick();
    for (;;) {
        {
            detail::Gate lock(gate);
            if (!lock) return Status::Busy;
            if (!online) return Status::NotInitialized;
            if (transferMode == TransferMode::Interrupt) {
                const auto fault = rxFault.exchange(Status::Ok);
                if (fault != Status::Ok) return fault;
                const auto index = tail.load();
                if (index != head.load()) {
                    output = ring[index % ring.size()];
                    tail.store(index + 1);
                    return Status::Ok;
                }
            } else {
                if (__HAL_CAN_GET_FLAG(&hcan, CAN_FLAG_FOV0)) {
                    __HAL_CAN_CLEAR_FLAG(&hcan, CAN_FLAG_FOV0);
                    return Status::Overflow;
                }
                if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0)) return readFifo(output);
            }
            if (hcan.Instance->ESR & CAN_ESR_BOFF) return Status::BusError;
        }
        if (detail::expired(start, ms)) return Status::Timeout;
        detail::nextTick();
    }
}

Status cancel(Ticket ticket) {
    detail::Gate lock(gate);
    if (!lock) return Status::Busy;
    return tx.owns(ticket) ? setup(internalLoopback, transferMode) : Status::Cancelled;
}
Status recover() {
    detail::Gate lock(gate);
    return lock ? setup(internalLoopback, transferMode) : Status::Busy;
}

// HAL invokes these through the CubeMX CAN1 interrupt handlers.
void onRx(CAN_HandleTypeDef* peripheral) {
    if (peripheral != &hcan) return;
    const auto n = HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0);
    for (uint32_t i = 0; i < n; ++i) {
        Frame frame{};
        const auto rc = readFifo(frame);
        if (rc != Status::Ok) { rxFault.store(rc); break; }
        const auto index = head.load();
        if (index - tail.load() >= ring.size()) { rxFault.store(Status::Overflow); continue; }
        ring[index % ring.size()] = frame;
        head.store(index + 1);
    }
}
void onTx(CAN_HandleTypeDef* peripheral, Status status) {
    if (peripheral == &hcan) irqTx.store(status);
}
void onError(CAN_HandleTypeDef* peripheral) {
    if (peripheral != &hcan) return;
    const uint32_t code = HAL_CAN_GetError(&hcan);
    if (code & HAL_CAN_ERROR_RX_FOV0) rxFault.store(Status::Overflow);
    if (code & (txErrors | HAL_CAN_ERROR_BOF)) irqTx.store(Status::BusError);
    // HAL accumulates errors. Consume them here so a later RX-only error cannot fail a new TX.
    (void)HAL_CAN_ResetError(&hcan);
}
}

extern "C" {
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* c) { fw::can::onRx(c); }
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* c) { fw::can::onTx(c, fw::Status::Ok); }
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* c) { fw::can::onTx(c, fw::Status::Ok); }
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* c) { fw::can::onTx(c, fw::Status::Ok); }
void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef* c) { fw::can::onTx(c, fw::Status::Cancelled); }
void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef* c) { fw::can::onTx(c, fw::Status::Cancelled); }
void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef* c) { fw::can::onTx(c, fw::Status::Cancelled); }
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* c) { fw::can::onError(c); }
}
