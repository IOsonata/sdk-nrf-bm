/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Modified for IOsonata - Zephyr dependencies removed
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>

#include <nrf_soc.h>

#include "bm/softdevice_handler/nrf_sdh.h"
#include "bm/softdevice_handler/nrf_sdh_soc.h"

/*
 * The SoftDevice requires SD_RAND_SEED_SIZE bytes of true random
 * entropy whenever it fires NRF_EVT_RAND_SEED_REQUEST.
 *
 * The original Nordic code uses cracen_get_trng() from the PSA Crypto
 * driver in sdk-nrf. That repo is not part of the bare-metal toolchain,
 * so we use the nrfx CRACEN CTR-DRBG driver directly instead.
 */
#include <nrfx_cracen.h>

/* Extern in nrf_sdh.c (called directly at enable time for scheduler model) */
void sdh_soc_rand_seed(uint32_t evt, void *ctx)
{
	uint32_t nrf_err;
	uint8_t seed[SD_RAND_SEED_SIZE];

	(void)ctx;

	if (evt != NRF_EVT_RAND_SEED_REQUEST) {
		return;
	}

	if (nrfx_cracen_entropy_get(seed, sizeof(seed)) != 0) {
		return;
	}

	nrf_err = sd_rand_seed_set(seed);
	(void)nrf_err;

	/* Discard seed immediately */
	memset(seed, 0, sizeof(seed));
}

/* Auto-handle seed requests as a SoC event observer */
NRF_SDH_SOC_OBSERVER(rand_seed, sdh_soc_rand_seed, NULL, HIGH);
