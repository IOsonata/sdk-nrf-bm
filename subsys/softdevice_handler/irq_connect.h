/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define IRQ_FORWARDING_ENABLED_MAGIC_NUMBER 0x47F34BC1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize SVC/IRQ forwarding to SoftDevice (phase 1).
 *
 * Relocates vector table to RAM, patches SVC/HardFault forwarding,
 * and sets the SD base address.  Must be called before any SoftDevice
 * SVCALL (e.g. sd_softdevice_is_enabled).
 *
 * @return 0 on success.
 */
int sd_irq_init(void);

/**
 * @brief Run the SoftDevice reset handler.
 *
 * Initializes SD internal RAM state.  Must be called after sd_irq_init()
 * and before sd_softdevice_enable().
 */
void CallSoftDeviceResetHandler(void);

/**
 * @brief Connect SD-owned peripheral IRQs (phase 2).
 *
 * Must be called AFTER sd_softdevice_enable() returns successfully.
 * Sets NVIC priorities for SD-owned interrupts and enables the
 * forwarding path for peripheral IRQs.
 */
void sd_irq_post_enable(void);

#ifdef __cplusplus
}
#endif
