/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "bm/softdevice_handler/nrf_sdh.h"
#include "bm/bm_scheduler.h"
#include "bm_compat.h"
/* nrf_sdh_config.h removed — all CONFIG_NRF_SDH_* values are provided
 * by bm_config_defaults.h, which bm_compat.h includes automatically. */
#include "coredev/system_core_clock.h"
#include "irq_connect.h"

#include "idelay.h"

extern McuOsc_t g_McuOsc;

/* Forward declaration — defined below, needed by nrf_sdh_enable() */
static int sd_direct_isr(void);


/* Clock source is determined at runtime from g_McuOsc.LowPwrOsc.Type,
 * so compile-time BUILD_ASSERTs for LFXO/RC are not applicable.
 * The rc_ctiv and rc_temp_ctiv values are set dynamically in nrf_sdh_enable().
 */

#if defined(CONFIG_NRF_GRTC_TIMER)
BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_GRTC_START_SYSCOUNTER),
	     "The application must start the GRTC for the SoftDevice to have a clock source");
BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_GRTC_TIMER_SOURCE_LFXO) ||
	     IS_ENABLED(CONFIG_NRF_GRTC_TIMER_SOURCE_LFLPRC),
	     "The selected GRTC timer source is invalid when using SoftDevice. "
	     "Please select either LFXO (if external LF oscillator) or LFLPRC (internal RC)");
#endif /* CONFIG_NRF_GRTC_TIMER */

static atomic_t sdh_is_suspended;	/* Whether the SoftDevice event interrupts are disabled. */
static atomic_t sdh_transition;		/* Whether enable/disable process was started. */

static char *state_to_str(enum nrf_sdh_state_evt s)
{
	switch (s) {
	case NRF_SDH_STATE_EVT_ENABLE_PREPARE:
		return "enabling";
	case NRF_SDH_STATE_EVT_ENABLED:
		return "enabled";
	case NRF_SDH_STATE_EVT_DISABLE_PREPARE:
		return "disabling";
	case NRF_SDH_STATE_EVT_DISABLED:
		return "disabled";
	default:
		return "unknown";
	};
}

/**
 * @brief Notify a state change to state observers.
 *
 * Extern in nrf_sdh_ble.c
 *
 * @param state The state to be notified.
 * @return true If any observers are busy.
 * @return false If no observers are busy.
 */
bool sdh_state_evt_observer_notify(enum nrf_sdh_state_evt state)
{
	bool busy;
	bool all_ready;
	bool busy_is_allowed;

	all_ready = true;
	busy_is_allowed = (state == NRF_SDH_STATE_EVT_ENABLE_PREPARE) ||
			  (state == NRF_SDH_STATE_EVT_DISABLE_PREPARE);

	TYPE_SECTION_FOREACH(struct nrf_sdh_state_evt_observer, nrf_sdh_state_evt_observers, obs) {
		/* If it's a _PREPARE event, dispatch only to busy observers, and update
		 * their busy state in RAM. Otherwise dispatch unconditionally to all observers.
		 */
		if (busy_is_allowed && obs->is_busy) {
			obs->is_busy = !!obs->handler(state, obs->context);
			all_ready &= !obs->is_busy;
		} else {
			busy = obs->handler(state, obs->context);
		}
	}

	return !all_ready;
}

__attribute__((weak)) void softdevice_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
#if 0
	switch (id) {
	case NRF_FAULT_ID_SD_ASSERT:
		LOG_ERR("NRF_FAULT_ID_SD_ASSERT: SoftDevice assert");
		break;
	case NRF_FAULT_ID_APP_MEMACC:
		LOG_ERR("NRF_FAULT_ID_APP_MEMACC: Application bad memory access");
		if (info == 0x00) {
			LOG_ERR("Application tried to access SoftDevice RAM");
		} else {
			LOG_ERR("Application tried to access SoftDevice peripheral at %#x", info);
		}
		break;
	}
#endif

	for (;;) {
		/* loop */
		__WFE();
	}
}

uint8_t nrf_get_lfclk_accuracy(uint32_t ppm)
{
	if (ppm < 2)
	{
		return 11;
	}
	else if (ppm < 5)
	{
		return 10;
	}
	else if (ppm < 10)
	{
		return 9;
	}
	else if (ppm < 20)
	{
		return 8;
	}
	else if (ppm < 30)
	{
		return 7;
	}
	else if (ppm < 50)
	{
		return 6;
	}
	else if (ppm < 75)
	{
		return 5;
	}
	else if (ppm < 100)
	{
		return 4;
	}
	else if (ppm < 150)
	{
		return 3;
	}
	else if (ppm < 250)
	{
		return 2;
	}
	else if (ppm < 500)
	{
		return 1;	/* NRF_CLOCK_LF_ACCURACY_250_PPM */
	}
	else
	{
		return 0;	/* NRF_CLOCK_LF_ACCURACY_500_PPM */
	}
}

static int nrf_sdh_enable(void)
{
	int err;
	const bool is_xo = (g_McuOsc.LowPwrOsc.Type != 0);  /* 0 = RC, 1 = LFXO */
	const nrf_clock_lf_cfg_t clock_lf_cfg = {
		.source = g_McuOsc.LowPwrOsc.Type, //CONFIG_NRF_SDH_CLOCK_LF_SRC,
		.rc_ctiv = is_xo ? 0 : CONFIG_NRF_SDH_CLOCK_LF_RC_CTIV,
		.rc_temp_ctiv = is_xo ? 0 : CONFIG_NRF_SDH_CLOCK_LF_RC_TEMP_CTIV,
		.accuracy = nrf_get_lfclk_accuracy(g_McuOsc.LowPwrOsc.Accuracy), //CONFIG_NRF_SDH_CLOCK_LF_ACCURACY,
		.hfclk_latency = CONFIG_NRF_SDH_CLOCK_HFCLK_LATENCY,
		.hfint_ctiv = CONFIG_NRF_SDH_CLOCK_HFINT_CALIBRATION_INTERVAL,
	};

	/* S145 requires GRTC SYSCOUNTER running before sd_softdevice_enable.
	 * Match the proven IOsonata SDC/MPSL init sequence (nrf_mpsl.cpp:96-97):
	 *   NRF_GRTC->MODE |= 2;
	 *   NRF_GRTC->TASKS_START = 1;
	 * Only set SYSCOUNTEREN (bit 1). Do NOT set AUTOEN, do NOT touch
	 * CLKCFG, do NOT call TASKS_STOP. The SD handles clock configuration
	 * internally via the clock_lf_cfg passed to sd_softdevice_enable(). */
	NRF_GRTC->MODE |= 1;		/* SYSCOUNTEREN */
	NRF_GRTC->TASKS_START = 1;

	/* Sanitize NVIC state for SoftDevice. */
	{
		static const IRQn_Type sd_irqs[] = {
			RADIO_0_IRQn, TIMER10_IRQn, GRTC_3_IRQn,
			AAR00_CCM00_IRQn, ECB00_IRQn, CLOCK_POWER_IRQn,
			SWI00_IRQn, SWI01_IRQn, SWI02_IRQn
		};
		for (unsigned i = 0; i < sizeof(sd_irqs)/sizeof(sd_irqs[0]); i++) {
			NVIC_DisableIRQ(sd_irqs[i]);
			NVIC_ClearPendingIRQ(sd_irqs[i]);
			NVIC_SetPriority(sd_irqs[i], 4);
		}
		for (int irqn = 0; irqn < 240; irqn++) {
			if (NVIC_GetEnableIRQ((IRQn_Type)irqn)) {
				uint32_t prio = NVIC_GetPriority((IRQn_Type)irqn);
				if (prio < 2) {
					NVIC_SetPriority((IRQn_Type)irqn, 2);
				}
			}
		}
	}
msDelay(10);

	printf("GRTC: MODE=0x%x\r\n", NRF_GRTC->MODE);

	err = sd_softdevice_enable(&clock_lf_cfg, softdevice_fault_handler);
	if (err) {
		printf("sd_softdevice_enable failed: SD err 0x%x\r\n", err);
		return -EINVAL;
	}

	/* Phase 2: now that the SD is running, connect its peripheral IRQs
	 * and enable the forwarding path.  Must be after sd_softdevice_enable
	 * because the SD validates interrupt config during enable. */
	sd_irq_post_enable();

	atomic_set(&sdh_is_suspended, false);
	atomic_set(&sdh_transition, false);

	/* The SoftDevice requires RNG seeding after enable, before sd_ble_enable().
	 * Without this, sd_ble_enable() returns 0x8 (NRF_ERROR_INVALID_STATE).
	 * In Zephyr this was gated behind DISPATCH_MODEL_SCHED, but in bare-metal
	 * we must always do it synchronously here. */
	{
		extern void sdh_soc_rand_seed(uint32_t evt, void *ctx);
		(void) sdh_soc_rand_seed(NRF_EVT_RAND_SEED_REQUEST, NULL);
	}

	/* Enable event interrupt.
	 * SYS_INIT is stripped in bare-metal, so wire the SD event IRQ here
	 * (was originally done by SYS_INIT(sd_irq_init) at boot). */
	IRQ_DIRECT_CONNECT(SD_EVT_IRQn, 4, sd_direct_isr, 0);
	NVIC_EnableIRQ((IRQn_Type)SD_EVT_IRQn);

	(void)sdh_state_evt_observer_notify(NRF_SDH_STATE_EVT_ENABLED);

	return 0;
}

static int nrf_sdh_disable(void)
{
	int err;

	err = sd_softdevice_disable();
	if (err) {
		LOG_ERR("Failed to disable SoftDevice, nrf_error %#x", err);
		return -EINVAL;
	}

	atomic_set(&sdh_transition, false);

	NVIC_DisableIRQ((IRQn_Type)SD_EVT_IRQn);

	(void)sdh_state_evt_observer_notify(NRF_SDH_STATE_EVT_DISABLED);

	return 0;
}

int nrf_sdh_enable_request(void)
{
	bool busy;

	/* Handle warm reset (debugger, watchdog): SRAM is retained and
	 * the SD's "enabled" flag from the previous session survives.
	 * Zero the SD's static+dynamic RAM region to clear stale state
	 * instead of calling sd_softdevice_disable() which tears down
	 * GRTC and other peripherals we can't fully reconstruct.
	 *
	 * SD RAM layout (from DTS):
	 *   Static:  0x20000000 - 0x2000177F  (6K)
	 *   Dynamic: 0x20001780 - 0x2000477F  (12K)
	 */
	memset((void *)0x20000000, 0, 0x4780);

	if (sdh_transition) {
		return -EINPROGRESS;
	}

	atomic_set(&sdh_transition, true);

	/* Assume all observers to be busy */
	TYPE_SECTION_FOREACH(struct nrf_sdh_state_evt_observer,
			     nrf_sdh_state_evt_observers, obs) {
		obs->is_busy = true;
	}

	busy = sdh_state_evt_observer_notify(NRF_SDH_STATE_EVT_ENABLE_PREPARE);
	if (busy) {
		/* Leave sdh_transition to 1, so process can be continued */
		return -EBUSY;
	}

	return nrf_sdh_enable();
}

int nrf_sdh_disable_request(void)
{
	bool busy;
	uint8_t enabled;

	(void)sd_softdevice_is_enabled(&enabled);
	if (!enabled) {
		return -EALREADY;
	}

	if (sdh_transition) {
		return -EINPROGRESS;
	}

	atomic_set(&sdh_transition, true);

	/* Assume all observers to be busy */
	TYPE_SECTION_FOREACH(struct nrf_sdh_state_evt_observer,
			     nrf_sdh_state_evt_observers, obs) {
		obs->is_busy = true;
	}

	busy = sdh_state_evt_observer_notify(NRF_SDH_STATE_EVT_DISABLE_PREPARE);
	if (busy) {
		/* Leave sdh_transition to 1, so process can be continued */
		return -EBUSY;
	}

	return nrf_sdh_disable();
}

int nrf_sdh_observer_ready(struct nrf_sdh_state_evt_observer *obs)
{
	int err;
	bool busy;
	uint8_t enabled;

	if (!obs) {
		return -EFAULT;
	}
	if (!sdh_transition) {
		return -EPERM;
	}
	if (!obs->is_busy) {
		LOG_WRN("Observer %p is not busy", obs);
		return 0;
	}

	obs->is_busy = false;

	(void)sd_softdevice_is_enabled(&enabled);

	busy = sdh_state_evt_observer_notify(enabled ? NRF_SDH_STATE_EVT_DISABLE_PREPARE
						     : NRF_SDH_STATE_EVT_ENABLE_PREPARE);

	/* Another observer needs to ready itself */
	if (busy) {
		return 0;
	}

	if (enabled) {
		err = nrf_sdh_disable();
	} else {
		err = nrf_sdh_enable();
	}

	__ASSERT(!err, "Failed to change SoftDevice state");
	(void) err;

	return 0;
}

void nrf_sdh_suspend(void)
{
	uint8_t sd_is_enabled;

	(void)sd_softdevice_is_enabled(&sd_is_enabled);

	if (!sd_is_enabled) {
		LOG_WRN("Tried to suspend, but SoftDevice is disabled");
		return;
	}
	if (sdh_is_suspended) {
		LOG_WRN("Tried to suspend, but already is suspended");
		return;
	}

	NVIC_DisableIRQ((IRQn_Type)SD_EVT_IRQn);

	atomic_set(&sdh_is_suspended, true);
}

void nrf_sdh_resume(void)
{
	uint8_t sd_is_enabled;

	(void)sd_softdevice_is_enabled(&sd_is_enabled);

	if (!sd_is_enabled) {
		LOG_WRN("Tried to resume, but SoftDevice is disabled");
		return;
	}
	if (!sdh_is_suspended) {
		LOG_WRN("Tried to resume, but not suspended");
		return;
	}

	/* Force calling ISR again to make sure we dispatch pending events */
	NVIC_SetPendingIRQ((IRQn_Type)SD_EVT_IRQn);
	NVIC_EnableIRQ((IRQn_Type)SD_EVT_IRQn);

	atomic_set(&sdh_is_suspended, false);
}

bool nrf_sdh_is_suspended(void)
{
	return sdh_is_suspended;
}

void nrf_sdh_evts_poll(void)
{
	/* Notify observers about pending SoftDevice event. */
	TYPE_SECTION_FOREACH(struct nrf_sdh_stack_evt_observer, nrf_sdh_stack_evt_observers, obs) {
		obs->handler(obs->context);
	}
}

#if defined(CONFIG_NRF_SDH_DISPATCH_MODEL_IRQ)

void SD_EVT_IRQHandler(void)
{
	nrf_sdh_evts_poll();
}

#elif defined(CONFIG_NRF_SDH_DISPATCH_MODEL_SCHED)

static void sdh_events_poll(void *data, size_t len)
{
	(void)data;
	(void)len;

	nrf_sdh_evts_poll();
}

void SD_EVT_IRQHandler(void)
{
	int err;

	err = bm_scheduler_defer(sdh_events_poll, NULL, 0);
	if (err) {
		LOG_WRN("Unable to schedule SoftDevice event, err %d", err);
	}
}

#elif defined(CONFIG_NRF_SDH_DISPATCH_MODEL_POLL)

#endif /* NRF_SDH_DISPATCH_MODEL */

ISR_DIRECT_DECLARE(sd_direct_isr)
{
	SD_EVT_IRQHandler();
	return 0;
}

/* sd_irq_init() functionality (SD_EVT_IRQn wiring) is now inlined
 * in nrf_sdh_enable() above.  The original SYS_INIT(sd_irq_init, ...)
 * was stripped by bm_compat.h; inlining avoids the silent no-op. */
