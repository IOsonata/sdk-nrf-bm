/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * SoftDevice ISR forwarding and IRQ initialization for nRF54L.
 *
 * Merged irq_connect.c + irq_forward.s into a single compilation unit
 * so that when the linker pulls in sd_irq_init() (called from BtAppInit),
 * the strong SVC_Handler / HardFault_Handler / peripheral ISR definitions
 * come along and override the weak defaults in vector_nrf54l15.c.
 *
 * Additionally, sd_irq_init() patches the vector table at runtime as a
 * belt-and-suspenders guarantee against linker archive resolution issues.
 *
 * Flow:
 *   CPU takes SVC/IRQ → enters naked handler →
 *     if SD enabled: reads SD's vector at softdevice_vector_forward_address,
 *                    BX to SD handler (tail-call, exception frame intact)
 *     if SD disabled: BX to app's C_xxx_Handler fallback
 */

#include <stdint.h>
#include "bm_compat.h"

#if CONFIG_SOC_SERIES_NRF54L

#include "irq_connect.h"
#include "nrf_sd_isr.h"

/* Stringify helpers — expand macro value THEN stringify */
#define _SD_XSTR(x) #x
#define SD_XSTR(x)  _SD_XSTR(x)


/* ======================================================================
 * Data — referenced by naked asm handlers
 * ====================================================================== */

uint32_t irq_forwarding_enabled_magic_number_holder;
uint32_t softdevice_vector_forward_address;


/* ======================================================================
 * ISR Forwarding Handlers (naked — no compiler prologue/epilogue)
 *
 * The SoftDevice handlers expect the hardware exception frame
 * {R0-R3, R12, LR, PC, xPSR} untouched on the stack.
 * ====================================================================== */

/* ------------------------------------------------------------------
 * ConsumeOrForwardIRQ (internal)
 *
 * On entry (set by caller):
 *   R0 = NRF_SD_ISR_OFFSET_xxx  (SD vector table offset)
 *   R3 = address of app-side C_xxx_Handler fallback
 *
 * If SD forwarding active  → BX to SD handler
 * If SD forwarding inactive → BX R3 (app handler)
 * ------------------------------------------------------------------ */
__attribute__((naked, used))
static void ConsumeOrForwardIRQ(void)
{
	__asm volatile(
		"LDR  R2, =irq_forwarding_enabled_magic_number_holder \n"
		"LDR  R2, [R2]                                        \n"
		"LDR  R1, =" SD_XSTR(IRQ_FORWARDING_ENABLED_MAGIC_NUMBER) "\n"
		"CMP  R2, R1                                          \n"
		"BNE  1f                                              \n"
		/* SD enabled — forward */
		"LDR  R1, =softdevice_vector_forward_address          \n"
		"LDR  R1, [R1]                                        \n"
		"LDR  R1, [R1, R0]                                    \n"
		"BX   R1                                              \n"
		"1:                                                   \n"
		/* SD disabled — app handler */
		"BX   R3                                              \n"
	);
}

/* ------------------------------------------------------------------
 * SVC_Handler — always forwards to SoftDevice.
 * (SVCs with SD numbers only issued after sd_softdevice_enable)
 * ------------------------------------------------------------------ */
__attribute__((naked))
void SVC_Handler(void)
{
	__asm volatile(
		"LDR  R0, =" SD_XSTR(NRF_SD_ISR_OFFSET_SVC) "        \n"
		"LDR  R1, =softdevice_vector_forward_address          \n"
		"LDR  R1, [R1]                                        \n"
		"LDR  R1, [R1, R0]                                    \n"
		"BX   R1                                              \n"
	);
}

/* ------------------------------------------------------------------
 * CallSoftDeviceResetHandler — called from sd_irq_init() to run
 * the SD's reset/init routine.  Not an ISR; uses BLX (call).
 * ------------------------------------------------------------------ */
__attribute__((naked))
void CallSoftDeviceResetHandler(void)
{
	__asm volatile(
		"LDR  R0, =" SD_XSTR(NRF_SD_ISR_OFFSET_RESET) "      \n"
		"LDR  R1, =softdevice_vector_forward_address          \n"
		"LDR  R1, [R1]                                        \n"
		"LDR  R1, [R1, R0]                                    \n"
		"PUSH {LR}                                            \n"
		"BLX  R1                                              \n"
		"POP  {LR}                                            \n"
		"BX   LR                                              \n"
	);
}

/* ------------------------------------------------------------------
 * Peripheral ISR forwarder macro
 * ------------------------------------------------------------------ */
#define SD_IRQ_FORWARDER(handler_name, c_fallback_name, sd_offset_macro) \
	__attribute__((naked))                                          \
	void handler_name(void)                                         \
	{                                                               \
		__asm volatile(                                         \
			"LDR R3, =" #c_fallback_name "            \n"   \
			"LDR R0, =" SD_XSTR(sd_offset_macro) "    \n"   \
			"B   ConsumeOrForwardIRQ                   \n"   \
		);                                                      \
	}

SD_IRQ_FORWARDER(HardFault_Handler,         C_HardFault_Handler,       NRF_SD_ISR_OFFSET_HARDFAULT)
SD_IRQ_FORWARDER(CLOCK_POWER_SD_IRQHandler, C_CLOCK_POWER_SD_Handler,  NRF_SD_ISR_OFFSET_CLOCK_POWER)
SD_IRQ_FORWARDER(RADIO_0_IRQHandler,        C_RADIO_0_Handler,         NRF_SD_ISR_OFFSET_RADIO_0)
SD_IRQ_FORWARDER(TIMER10_IRQHandler,        C_TIMER10_Handler,         NRF_SD_ISR_OFFSET_TIMER10)
SD_IRQ_FORWARDER(GRTC_3_IRQHandler,         C_GRTC_3_Handler,         NRF_SD_ISR_OFFSET_GRTC_3)
SD_IRQ_FORWARDER(ECB00_IRQHandler,          C_ECB00_Handler,           NRF_SD_ISR_OFFSET_ECB00)
SD_IRQ_FORWARDER(AAR00_CCM00_IRQHandler,    C_AAR00_CCM00_Handler,     NRF_SD_ISR_OFFSET_AAR00_CCM00)
SD_IRQ_FORWARDER(SWI00_IRQHandler,          C_SWI00_Handler,           NRF_SD_ISR_OFFSET_SWI00)


/* ======================================================================
 * Vector table relocation to RAM
 *
 * The default vector table lives in RRAM (.vectors section).  RRAM on
 * nRF54L15 is not directly writable with plain stores — it needs
 * RRAMC configuration.  Rather than enabling RRAMC writes, we copy
 * the entire table into a RAM buffer and re-point SCB->VTOR.
 * All subsequent writes (sd_patch_system_vectors, NVIC_SetVector for
 * IRQ_DIRECT_CONNECT) then target writable SRAM.
 *
 * nRF54L15 vector table has ~289 entries.  We round up to 512 entries
 * (2048 bytes) so the VTOR alignment requirement is satisfied (table
 * base must be aligned to its size rounded up to the next power of 2).
 * ====================================================================== */

#define VTOR_TABLE_ENTRIES  512

static uint32_t s_ram_vectors[VTOR_TABLE_ENTRIES]
	__attribute__((aligned(2048)));

static void sd_relocate_vectors_to_ram(void)
{
	const uint32_t *src = (const uint32_t *)SCB->VTOR;

	for (int i = 0; i < VTOR_TABLE_ENTRIES; i++)
		s_ram_vectors[i] = src[i];

	SCB->VTOR = (uint32_t)s_ram_vectors;
	__DSB();
	__ISB();
}


/* ======================================================================
 * Runtime vector table patching
 *
 * NVIC_SetVector only handles peripheral IRQs (positive IRQ numbers).
 * System exceptions like SVC and HardFault must be patched directly
 * in the vector table via VTOR.
 * ====================================================================== */

/* Exception positions in the ARM vector table */
#define VTOR_POS_HARDFAULT  3   /* offset 0x0C */
#define VTOR_POS_SVC       11   /* offset 0x2C */

static void sd_patch_system_vectors(void)
{
	uint32_t *vtor = (uint32_t *)SCB->VTOR;

	/* Patch SVC_Handler and HardFault_Handler entries.
	 * The Thumb bit is included automatically by the function address. */
	vtor[VTOR_POS_SVC]       = (uint32_t)SVC_Handler;
	vtor[VTOR_POS_HARDFAULT] = (uint32_t)HardFault_Handler;

	/* Ensure writes are visible before any SVC instruction */
	__DSB();
	__ISB();
}


/* ======================================================================
 * Initialization — two-phase to bracket sd_softdevice_enable():
 *
 *   sd_irq_init()             ← before sd_softdevice_enable()
 *     Relocate vectors to RAM, patch SVC/HardFault forwarding,
 *     set SD base address, run SD reset handler.
 *
 *   sd_irq_post_enable()      ← after sd_softdevice_enable()
 *     Connect SD-owned peripheral IRQs with correct priorities,
 *     enable the forwarding magic number.
 *
 * The split is required because our bare-metal IRQ_DIRECT_CONNECT
 * calls NVIC_SetPriority at runtime.  The SD validates interrupt
 * config during sd_softdevice_enable and will return
 * NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION (0x1001) if
 * its owned IRQs already have priorities set.
 * ====================================================================== */

#define PRIO_HIGH 0   /* SoftDevice high priority interrupt */
#define PRIO_LOW  4   /* SoftDevice low priority interrupt */

int sd_irq_init(void)
{
	/* 0. Move the vector table from RRAM to SRAM so that all
	 *    subsequent writes (system vector patches, NVIC_SetVector)
	 *    target writable memory. */
	sd_relocate_vectors_to_ram();

	/* 1. Patch SVC_Handler and HardFault_Handler into the vector table.
	 *    Guarantees the forwarding handlers are active regardless of
	 *    how the linker resolved weak vs strong symbols at link time. */
	sd_patch_system_vectors();

	/* 2. Set SD base address so SVC forwarding knows where to jump.
	 *    Do NOT call CallSoftDeviceResetHandler() here — it makes
	 *    sd_softdevice_is_enabled() return true, which causes
	 *    nrf_sdh_enable_request() to skip sd_softdevice_enable().
	 *    The reset handler is called in nrf_sdh_enable() instead. */
	softdevice_vector_forward_address = FIXED_PARTITION_OFFSET(softdevice_partition);
#ifdef CONFIG_BOOTLOADER_MCUBOOT
	softdevice_vector_forward_address += CONFIG_ROM_START_OFFSET;
#endif

	return 0;
}

void sd_irq_post_enable(void)
{
	/* Wire SD-owned peripheral IRQs into NVIC.
	 * Must happen AFTER sd_softdevice_enable() — the SD validates
	 * that its IRQs are unconfigured during enable. */
	IRQ_DIRECT_CONNECT(RADIO_0_IRQn,     PRIO_HIGH, RADIO_0_IRQHandler,        IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(TIMER10_IRQn,     PRIO_HIGH, TIMER10_IRQHandler,        IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(GRTC_3_IRQn,      PRIO_HIGH, GRTC_3_IRQHandler,        IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(AAR00_CCM00_IRQn, PRIO_LOW,  AAR00_CCM00_IRQHandler,    0);
	IRQ_DIRECT_CONNECT(CLOCK_POWER_IRQn, PRIO_LOW,  CLOCK_POWER_SD_IRQHandler, 0);
	IRQ_DIRECT_CONNECT(ECB00_IRQn,       PRIO_LOW,  ECB00_IRQHandler,          0);
	IRQ_DIRECT_CONNECT(SWI00_IRQn,       PRIO_LOW,  SWI00_IRQHandler,          0);

	NVIC_SetPriority(SVCall_IRQn, PRIO_LOW);

	/* Enable the forwarding path in ConsumeOrForwardIRQ */
	irq_forwarding_enabled_magic_number_holder = IRQ_FORWARDING_ENABLED_MAGIC_NUMBER;
}


/* ======================================================================
 * App-side fallback handlers (weak — used when SD is disabled)
 * ====================================================================== */

__attribute__((weak)) void C_HardFault_Handler(void)
{
	__asm__("SVC 255");
}

__attribute__((weak)) void C_TIMER10_Handler(void)
{
	__asm__("SVC 255");
}

__attribute__((weak)) void C_GRTC_3_Handler(void)
{
	__asm__("SVC 255");
}

__attribute__((weak)) void C_SWI00_Handler(void)
{
	__asm__("SVC 255");
}

__attribute__((weak)) void C_RADIO_0_Handler(void)
{
	__asm__("SVC 255");
}

__attribute__((weak)) void C_ECB00_Handler(void)
{
	__asm__("SVC 255");
}

__attribute__((weak)) void C_AAR00_CCM00_Handler(void)
{
	__asm__("SVC 255");
}

__attribute__((weak)) void C_CLOCK_POWER_SD_Handler(void)
{
#if defined(CONFIG_NRFX_POWER) || defined(CONFIG_NRFX_CLOCK)
	extern void CLOCK_POWER_IRQHandler(void);
	CLOCK_POWER_IRQHandler();
#else
	__asm__("SVC 255");
#endif
}

#endif /* CONFIG_SOC_SERIES_NRF54L */
