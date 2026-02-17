/**-------------------------------------------------------------------------
@file	bm_compat.h

@brief	sdk-nrf-bm compatibility layer for IOsonata bare-metal conversion of sdk-nrf-bm

Drop-in replacement for Zephyr API replacements for sdk-nrf-bm bare-metal conversion.
Each section has its own include guard and can be independently
disabled by defining the guard before including this header.

Modular design for forward compatibility:
  - Each sdk-nrf-bm release may add/remove Zephyr usage
  - Sections can be individually overridden or excluded
  - Version tag tracks which sdk-nrf-bm release this was built against

Target: sdk-nrf-bm v2.9.1 (nRF Connect SDK bare-metal)
Compiler: arm-none-eabi-gcc with -std=c17
Platform: Cortex-M33 (nRF54L series)

Usage:
  Replace all <zephyr/...> includes with:
    #include "bm_compat.h"

  To disable a section (e.g., you provide your own logging):
    #define BM_COMPAT_LOGGING_H__
    #include "bm_compat.h"

@author	IOsonata
@date	2025

@license MIT
----------------------------------------------------------------------------*/

#ifndef BM_COMPAT_H__
#define BM_COMPAT_H__

/** Version of sdk-nrf-bm this compat layer targets */
#define BM_COMPAT_SDK_NRF_BM_VERSION  "2.9.1"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* CONFIG_ defaults from Kconfig — override before including this header */
#include "bm_config_defaults.h"
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __arm__
#include "nrf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* ======================================================================
 * Section: Logging  —  replaces <zephyr/logging/log.h>, log_ctrl.h
 *
 * Stripped to no-ops.  Define BM_COMPAT_LOGGING_H__ before
 * including this header to provide your own implementation.
 * ====================================================================== */
#ifndef BM_COMPAT_LOGGING_H__
#define BM_COMPAT_LOGGING_H__

#define LOG_MODULE_REGISTER(...)
#define LOG_MODULE_DECLARE(...)

#define LOG_ERR(...)    ((void)0)
#define LOG_WRN(...)    ((void)0)
#define LOG_INF(...)    ((void)0)
#define LOG_DBG(...)    ((void)0)

#define LOG_HEXDUMP_ERR(...)  ((void)0)
#define LOG_HEXDUMP_WRN(...)  ((void)0)
#define LOG_HEXDUMP_INF(...)  ((void)0)
#define LOG_HEXDUMP_DBG(...)  ((void)0)

/* log_ctrl.h */
#define LOG_PANIC()     ((void)0)
#define LOG_INIT()      ((void)0)
#define log_flush()     ((void)0)

#endif /* BM_COMPAT_LOGGING_H__ */


/* ======================================================================
 * Section: Util  —  replaces <zephyr/sys/util.h>, util_macro.h
 * ====================================================================== */
#ifndef BM_COMPAT_UTIL_H__
#define BM_COMPAT_UTIL_H__

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef BYTES_TO_WORDS
#define BYTES_TO_WORDS(bytes)  (((bytes) + 3) / 4)
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, align)  (((x) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(x, align)  ((x) & ~((align) - 1))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d)  (((n) + (d) - 1) / (d))
#endif

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(val, lo, hi)  MIN(MAX(val, lo), hi)
#endif

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#ifndef BIT_MASK
#define BIT_MASK(n)  (BIT(n) - 1UL)
#endif

#ifndef IS_ALIGNED
#define IS_ALIGNED(x, align)  (((x) & ((align) - 1)) == 0)
#endif

#ifndef IN_RANGE
#define IN_RANGE(val, min, max)  ((val) >= (min) && (val) <= (max))
#endif

#ifndef CONCAT
#define _CONCAT2(a, b)  a ## b
#define CONCAT(a, b)    _CONCAT2(a, b)
#endif

#ifndef UNUSED_VARIABLE
#define UNUSED_VARIABLE(x)  ((void)(x))
#endif

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x)  ((void)(x))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#ifndef SIZEOF_FIELD
#define SIZEOF_FIELD(type, member)  sizeof(((type *)0)->member)
#endif

/**
 * IS_ENABLED — Zephyr's macro for testing boolean Kconfig options.
 *
 * Returns 1 if config_macro is defined to 1, 0 otherwise.
 * Uses the _ZZZZ1 comma-injection trick: when the macro argument
 * expands to 1, _ZZZZ1 injects a comma that shifts the argument
 * selection from the default 0 to 1.
 */
#ifndef IS_ENABLED
#define _ZZZZ1                            _ZIGN,
#define _IS_ENABLED_3(_ign, val, ...)     val
#define _IS_ENABLED_2(one_or_two)         _IS_ENABLED_3(one_or_two 1, 0)
#define _IS_ENABLED_1(x)                  _IS_ENABLED_2(_ZZZZ ## x)
#define IS_ENABLED(config_macro)          _IS_ENABLED_1(config_macro)
#endif

/* sys_clock.h constants */
#ifndef USEC_PER_SEC
#define USEC_PER_SEC   1000000UL
#endif

#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC  1000UL
#endif

/*
 * COND_CODE_1(flag, if_1_code, else_code)
 *
 * If flag expands to literal 1, expand to the contents of if_1_code
 * (must be parenthesized).  Otherwise expand to else_code contents.
 *
 * Used by nrf_sdh.h PRIO_LEVEL_ORD for priority level resolution.
 * Supports deep nesting — only the selected branch appears in output.
 *
 * Example:
 *   #define FOO 1
 *   COND_CODE_1(FOO, (yes), (no))   // expands to: yes
 *   COND_CODE_1(BAR, (yes), (no))   // expands to: no
 *
 * Implementation follows Zephyr's util_macro.h pattern exactly:
 *   _XXXX1 injects a comma to shift argument positions.
 *   __GET_ARG2_DEBRACKET selects & debrackets the chosen branch.
 */
#ifndef COND_CODE_1
#define _XXXX1 _YYYY,
#define __DEBRACKET(...)                __VA_ARGS__
#define __GET_ARG2_DEBRACKET(ign, val, ...)  __DEBRACKET val
#define __COND_CODE(one_or_two, if1, el)     __GET_ARG2_DEBRACKET(one_or_two if1, el)
#define Z_COND_CODE_1(flag, if1, el)         __COND_CODE(_XXXX##flag, if1, el)
#define COND_CODE_1(flag, if1, el)           Z_COND_CODE_1(flag, if1, el)
#endif

/* -------------------------------------------------------------------------
 * IS_EMPTY(...)  —  expands to 1 when __VA_ARGS__ is empty, 0 otherwise
 *
 * Uses the Jens Gustedt technique: four comma-detection probes whose
 * concatenated 0/1 results form 0001 only when the token list is truly
 * empty, matching the _IS_EMPTY_CASE_0001 sentinel.
 * Required by dis.c gatt_char() macro, among others.
 * ------------------------------------------------------------------------- */
#ifndef IS_EMPTY

#define _IS_EMPTY_TRIGGER_PAREN_(...) ,

#define _IS_EMPTY_ARG3(_0, _1, _2, ...) _2
#define _IS_EMPTY_HAS_COMMA(...)        _IS_EMPTY_ARG3(__VA_ARGS__, 1, 0)

#define _IS_EMPTY_PASTE5_(_0, _1, _2, _3, _4)  _0##_1##_2##_3##_4
#define _IS_EMPTY_PASTE5(_0, _1, _2, _3, _4)   _IS_EMPTY_PASTE5_(_0, _1, _2, _3, _4)

#define _IS_EMPTY_CASE_0001 ,

#define _IS_EMPTY_EVAL(_0, _1, _2, _3)  \
	_IS_EMPTY_HAS_COMMA(_IS_EMPTY_PASTE5(_IS_EMPTY_CASE_, _0, _1, _2, _3))

#define IS_EMPTY(...)                                                                    \
	_IS_EMPTY_EVAL(                                                                  \
		_IS_EMPTY_HAS_COMMA(__VA_ARGS__),                                        \
		_IS_EMPTY_HAS_COMMA(_IS_EMPTY_TRIGGER_PAREN_ __VA_ARGS__),              \
		_IS_EMPTY_HAS_COMMA(__VA_ARGS__ (/*empty*/)),                            \
		_IS_EMPTY_HAS_COMMA(_IS_EMPTY_TRIGGER_PAREN_ __VA_ARGS__ (/*empty*/)))

#endif /* IS_EMPTY */

#endif /* BM_COMPAT_UTIL_H__ */


/* ======================================================================
 * Section: Assert  —  replaces <zephyr/sys/__assert.h>
 * ====================================================================== */
#ifndef BM_COMPAT_ASSERT_H__
#define BM_COMPAT_ASSERT_H__

#ifndef __ASSERT
#define __ASSERT(cond, ...)  assert(cond)
#endif

#ifndef __ASSERT_NO_MSG
#define __ASSERT_NO_MSG(cond)  assert(cond)
#endif

#ifndef BUILD_ASSERT
#define BUILD_ASSERT(cond, ...)  _Static_assert(cond, "" __VA_ARGS__)
#endif

#endif /* BM_COMPAT_ASSERT_H__ */


/* ======================================================================
 * Section: Toolchain  —  replaces <zephyr/toolchain.h>
 * ====================================================================== */
#ifndef BM_COMPAT_TOOLCHAIN_H__
#define BM_COMPAT_TOOLCHAIN_H__

#ifndef __weak
#define __weak  __attribute__((weak))
#endif

#ifndef __packed
#define __packed  __attribute__((packed))
#endif

#ifndef __aligned
#define __aligned(x)  __attribute__((aligned(x)))
#endif

#ifndef __ALIGN
#define __ALIGN(x)  __attribute__((aligned(x)))
#endif

#ifndef __used
#define __used  __attribute__((used))
#endif

#ifndef __unused
#define __unused  __attribute__((unused))
#endif

#ifndef __fallthrough
#if __GNUC__ >= 7
#define __fallthrough  __attribute__((fallthrough))
#else
#define __fallthrough  ((void)0)
#endif
#endif

#ifndef __deprecated
#define __deprecated  __attribute__((deprecated))
#endif

#ifndef __printf_like
#define __printf_like(fmtidx, argidx) \
	__attribute__((format(printf, fmtidx, argidx)))
#endif

#ifndef ARG_UNUSED
#define ARG_UNUSED(x)  (void)(x)
#endif

#ifndef __INLINE
#define __INLINE  inline
#endif

#ifndef STRINGIFY
#define _STRINGIFY(x)  #x
#define STRINGIFY(x)   _STRINGIFY(x)
#endif

#endif /* BM_COMPAT_TOOLCHAIN_H__ */


/* ======================================================================
 * Section: IRQ  —  replaces <zephyr/irq.h>
 *
 * Maps to CMSIS NVIC.  Requires nrf.h (included above).
 * ====================================================================== */
#ifndef BM_COMPAT_IRQ_H__
#define BM_COMPAT_IRQ_H__

#ifndef irq_enable
#define irq_enable(irqn)   NVIC_EnableIRQ((IRQn_Type)(irqn))
#endif

#ifndef irq_disable
#define irq_disable(irqn)  NVIC_DisableIRQ((IRQn_Type)(irqn))
#endif

static inline unsigned int irq_lock(void)
{
	unsigned int key = __get_PRIMASK();
	__disable_irq();
	return key;
}

static inline void irq_unlock(unsigned int key)
{
	__set_PRIMASK(key);
}

/*
 * ISR_DIRECT_DECLARE — declare a direct ISR (Zephyr convention: returns int).
 *
 * On bare metal ARM Cortex-M the return value is unused.  The int return
 * is kept for source compatibility with sdk-nrf-bm ISR code.
 */
#ifndef ISR_DIRECT_DECLARE
#define ISR_DIRECT_DECLARE(name) static int name(void)
#endif

/*
 * IRQ_DIRECT_CONNECT — wire an ISR to a NVIC vector at runtime.
 *
 * Uses CMSIS NVIC_SetVector to relocate the vector table to RAM (on
 * first call) and install the handler.  The cast to uint32_t suppresses
 * the int-returning ISR vs void(*)(void) vector mismatch — on ARM the
 * stale R0 is harmless since the return from exception restores context.
 *
 * If IRQ_ZERO_LATENCY flag is set, priority is forced to 0 regardless
 * of the prio argument.
 */
#ifndef IRQ_ZERO_LATENCY
#define IRQ_ZERO_LATENCY  (1u << 0)
#endif

#ifndef IRQ_DIRECT_CONNECT
#define IRQ_DIRECT_CONNECT(irq, prio, isr, flags) do {                 \
	NVIC_SetVector((IRQn_Type)(irq), (uint32_t)(isr));                 \
	NVIC_SetPriority((IRQn_Type)(irq),                                 \
	                 ((flags) & IRQ_ZERO_LATENCY) ? 0 : (prio));       \
} while (0)
#endif

#endif /* BM_COMPAT_IRQ_H__ */


/* ======================================================================
 * Section: Kernel  —  replaces <zephyr/kernel.h> CPU primitives
 * ====================================================================== */
#ifndef BM_COMPAT_KERNEL_H__
#define BM_COMPAT_KERNEL_H__

/**
 * k_cpu_idle — enter low-power idle until next interrupt.
 * Maps to ARM WFE (Wait For Event) which is the standard
 * bare-metal idle pattern used by IOsonata's main loop.
 */
#ifndef k_cpu_idle
#ifdef __arm__
#define k_cpu_idle()  __WFE()
#else
#define k_cpu_idle()  do {} while (0)
#endif
#endif

/**
 * k_uptime_ticks — monotonic tick count since boot.
 *
 * Reads GRTC SYSCOUNTER (DevNo 0) directly — the same 16 MHz counter
 * that IOsonata's timer_lf and the bm_timer wrapper use.  Returns
 * ticks in BM_TIMER_TICK_FREQ units so elapsed calculations match
 * BM_TIMER_MS_TO_TICKS values.
 */
#ifdef __arm__
static inline uint64_t k_uptime_ticks(void)
{
	uint32_t cl, ch;

	do {
		cl = NRF_GRTC->SYSCOUNTER[0].SYSCOUNTERL;
		ch = NRF_GRTC->SYSCOUNTER[0].SYSCOUNTERH;
	} while (ch & GRTC_SYSCOUNTER_SYSCOUNTERH_BUSY_Msk);

	ch &= GRTC_SYSCOUNTER_SYSCOUNTERH_VALUE_Msk;

	return ((uint64_t)ch << 32) | (cl & GRTC_SYSCOUNTER_SYSCOUNTERL_VALUE_Msk);
}
#else
static inline uint64_t k_uptime_ticks(void) { return 0; }
#endif

/*
 * RTC / timer tick frequency and conversion macros.
 * Replaces <zephyr/sys/time_units.h>.
 *
 * bm_timer uses the 32.768 kHz RTC (LFCLK), not the 16 MHz GRTC.
 */
#ifndef TIMER_NRFX_RTC_BASE_FREQ
#define TIMER_NRFX_RTC_BASE_FREQ  32768
#endif

#ifndef CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC  TIMER_NRFX_RTC_BASE_FREQ
#endif

#define k_ms_to_ticks_floor32(ms) \
	((uint32_t)(((uint64_t)(ms) * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / 1000U))

#define k_ms_to_ticks_ceil32(ms) \
	((uint32_t)(((uint64_t)(ms) * CONFIG_SYS_CLOCK_TICKS_PER_SEC + 999U) / 1000U))

#define k_us_to_ticks_floor32(us) \
	((uint32_t)(((uint64_t)(us) * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / 1000000U))

#define k_us_to_ticks_ceil32(us) \
	((uint32_t)(((uint64_t)(us) * CONFIG_SYS_CLOCK_TICKS_PER_SEC + 999999U) / 1000000U))

#define k_ticks_to_ms_floor32(ticks) \
	((uint32_t)(((uint64_t)(ticks) * 1000U) / CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define k_ticks_to_us_floor32(ticks) \
	((uint32_t)(((uint64_t)(ticks) * 1000000U) / CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/**
 * k_busy_wait — spin for N microseconds.
 * Uses DWT cycle counter when available, NOP loop fallback.
 */
static inline void k_busy_wait(uint32_t usec)
{
#if defined(DWT) && defined(CoreDebug)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	uint32_t cycles = (SystemCoreClock / 1000000UL) * usec;
	while (DWT->CYCCNT < cycles) {
		/* spin */
	}
#else
	volatile uint32_t count = (SystemCoreClock / 4000000UL) * usec;
	while (count--) {
		__NOP();
	}
#endif
}

#ifndef printk
#define printk(...)  ((void)0)
#endif

#endif /* BM_COMPAT_KERNEL_H__ */


/* ======================================================================
 * Section: Byteorder  —  replaces <zephyr/sys/byteorder.h>
 *
 * ARM Cortex-M is little-endian.  LE accessors are identity.
 * ====================================================================== */
#ifndef BM_COMPAT_BYTEORDER_H__
#define BM_COMPAT_BYTEORDER_H__

static inline uint16_t sys_get_le16(const uint8_t src[2])
{
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static inline uint32_t sys_get_le32(const uint8_t src[4])
{
	return (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
	       ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static inline void sys_put_le16(uint16_t val, uint8_t dst[2])
{
	dst[0] = (uint8_t)(val);
	dst[1] = (uint8_t)(val >> 8);
}

static inline void sys_put_le32(uint32_t val, uint8_t dst[4])
{
	dst[0] = (uint8_t)(val);
	dst[1] = (uint8_t)(val >> 8);
	dst[2] = (uint8_t)(val >> 16);
	dst[3] = (uint8_t)(val >> 24);
}

static inline uint16_t sys_get_be16(const uint8_t src[2])
{
	return ((uint16_t)src[0] << 8) | (uint16_t)src[1];
}

static inline uint32_t sys_get_be32(const uint8_t src[4])
{
	return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
	       ((uint32_t)src[2] << 8) | (uint32_t)src[3];
}

static inline void sys_put_be16(uint16_t val, uint8_t dst[2])
{
	dst[0] = (uint8_t)(val >> 8);
	dst[1] = (uint8_t)(val);
}

static inline void sys_put_be32(uint32_t val, uint8_t dst[4])
{
	dst[0] = (uint8_t)(val >> 24);
	dst[1] = (uint8_t)(val >> 16);
	dst[2] = (uint8_t)(val >> 8);
	dst[3] = (uint8_t)(val);
}

/* LE platform — identity */
static inline uint16_t sys_le16_to_cpu(uint16_t val) { return val; }
static inline uint32_t sys_le32_to_cpu(uint32_t val) { return val; }
static inline uint16_t sys_cpu_to_le16(uint16_t val) { return val; }
static inline uint32_t sys_cpu_to_le32(uint32_t val) { return val; }

/* BE — byte swap */
static inline uint16_t sys_be16_to_cpu(uint16_t val)
{
	return (uint16_t)((val >> 8) | (val << 8));
}

static inline uint32_t sys_be32_to_cpu(uint32_t val)
{
	return ((val >> 24) & 0xFFu) | ((val >> 8) & 0xFF00u) |
	       ((val << 8) & 0xFF0000u) | ((val << 24) & 0xFF000000u);
}

static inline uint16_t sys_cpu_to_be16(uint16_t val) { return sys_be16_to_cpu(val); }
static inline uint32_t sys_cpu_to_be32(uint32_t val) { return sys_be32_to_cpu(val); }

/**
 * @brief Convert a uint64 value into a little-endian byte array initializer.
 *
 * Used by dis.c for CONFIG_BLE_DIS_REGULATORY_CERT_LIST.
 */
#ifndef sys_uint64_to_array
#define sys_uint64_to_array(val) {              \
	(uint8_t)((val) & 0xFF),                \
	(uint8_t)(((val) >> 8) & 0xFF),         \
	(uint8_t)(((val) >> 16) & 0xFF),        \
	(uint8_t)(((val) >> 24) & 0xFF),        \
	(uint8_t)(((val) >> 32) & 0xFF),        \
	(uint8_t)(((val) >> 40) & 0xFF),        \
	(uint8_t)(((val) >> 48) & 0xFF),        \
	(uint8_t)(((val) >> 56) & 0xFF),        \
}
#endif

#endif /* BM_COMPAT_BYTEORDER_H__ */


/* ======================================================================
 * Section: Atomic  —  replaces <zephyr/sys/atomic.h>
 *
 * Full C17 stdatomic implementation in separate header.
 * ====================================================================== */
#ifndef BM_ATOMIC_H__
#include "bm_atomic.h"
#endif


/* ======================================================================
 * Section: Init  —  replaces <zephyr/init.h>
 *
 * SYS_INIT() stripped.  Call init functions explicitly.
 * ====================================================================== */
#ifndef BM_COMPAT_INIT_H__
#define BM_COMPAT_INIT_H__

#ifndef SYS_INIT
#define SYS_INIT(fn, level, prio)  /* removed — call fn() explicitly */
#endif

#endif /* BM_COMPAT_INIT_H__ */


/* ======================================================================
 * Section: Singly-linked list  —  replaces <zephyr/sys/slist.h>
 * ====================================================================== */
#ifndef BM_COMPAT_SLIST_H__
#define BM_COMPAT_SLIST_H__

struct _snode {
	struct _snode *next;
};

typedef struct _snode sys_snode_t;

struct _slist {
	sys_snode_t *head;
	sys_snode_t *tail;
};

typedef struct _slist sys_slist_t;

/** @brief Static initializer for a singly-linked list. */
#define SYS_SLIST_STATIC_INIT(ptr_to_list) { .head = NULL, .tail = NULL }

static inline void sys_slist_init(sys_slist_t *list)
{
	list->head = NULL;
	list->tail = NULL;
}

static inline bool sys_slist_is_empty(sys_slist_t *list)
{
	return list->head == NULL;
}

static inline sys_snode_t *sys_slist_peek_head(sys_slist_t *list)
{
	return list->head;
}

static inline sys_snode_t *sys_slist_peek_next(sys_snode_t *node)
{
	return node ? node->next : NULL;
}

static inline void sys_slist_append(sys_slist_t *list, sys_snode_t *node)
{
	node->next = NULL;
	if (list->tail) {
		list->tail->next = node;
	} else {
		list->head = node;
	}
	list->tail = node;
}

static inline void sys_slist_prepend(sys_slist_t *list, sys_snode_t *node)
{
	node->next = list->head;
	list->head = node;
	if (list->tail == NULL) {
		list->tail = node;
	}
}

static inline sys_snode_t *sys_slist_get(sys_slist_t *list)
{
	sys_snode_t *node = list->head;
	if (node) {
		list->head = node->next;
		if (list->head == NULL) {
			list->tail = NULL;
		}
		node->next = NULL;
	}
	return node;
}

/** @brief Remove and return the first node. List must not be empty. */
static inline sys_snode_t *sys_slist_get_not_empty(sys_slist_t *list)
{
	sys_snode_t *node = list->head;

	list->head = node->next;
	if (list->head == NULL) {
		list->tail = NULL;
	}
	node->next = NULL;
	return node;
}

static inline void sys_slist_remove(sys_slist_t *list,
				    sys_snode_t *prev,
				    sys_snode_t *node)
{
	if (prev == NULL) {
		list->head = node->next;
	} else {
		prev->next = node->next;
	}
	if (list->tail == node) {
		list->tail = prev;
	}
	node->next = NULL;
}

static inline bool sys_slist_find_and_remove(sys_slist_t *list,
					     sys_snode_t *node)
{
	sys_snode_t *prev = NULL;
	sys_snode_t *curr = list->head;
	while (curr) {
		if (curr == node) {
			sys_slist_remove(list, prev, curr);
			return true;
		}
		prev = curr;
		curr = curr->next;
	}
	return false;
}

#define SYS_SLIST_FOR_EACH_NODE(list, node) \
	for ((node) = sys_slist_peek_head(list); \
	     (node) != NULL; \
	     (node) = (node)->next)

#define SYS_SLIST_FOR_EACH_NODE_SAFE(list, node, tmp) \
	for ((node) = sys_slist_peek_head(list), \
	     (tmp) = (node) ? (node)->next : NULL; \
	     (node) != NULL; \
	     (node) = (tmp), (tmp) = (node) ? (node)->next : NULL)

#endif /* BM_COMPAT_SLIST_H__ */


/* ======================================================================
 * Section: Barrier  —  replaces <zephyr/sys/barrier.h>
 * ====================================================================== */
#ifndef BM_COMPAT_BARRIER_H__
#define BM_COMPAT_BARRIER_H__

#ifndef barrier_dmem_fence_full
#define barrier_dmem_fence_full()   __DMB()
#endif

#ifndef barrier_isync_fence_full
#define barrier_isync_fence_full()  __ISB()
#endif

#endif /* BM_COMPAT_BARRIER_H__ */


/* ======================================================================
 * Section: CRC  —  replaces <zephyr/sys/crc.h>
 *
 * Zephyr convention: crc8_ccitt(seed, data, len)
 * IOsonata convention: crc8_ccitt(data, len, seed)
 *
 * We provide Zephyr-compatible inline wrappers here.
 * crc32_ieee(data, len) has no seed — compatible as-is.
 * ====================================================================== */
#ifndef BM_COMPAT_CRC_H__
#define BM_COMPAT_CRC_H__

#include "crc.h"

/*
 * If IOsonata crc8_ccitt signature is (data, len, seed), wrap it
 * to match Zephyr's (seed, data, len) calling convention used
 * throughout sdk-nrf-bm.  Rename the IOsonata function to avoid
 * conflict, then provide the Zephyr-ordered wrapper.
 *
 * NOTE: If IOsonata already uses Zephyr order, remove these wrappers.
 */

#endif /* BM_COMPAT_CRC_H__ */


/* ======================================================================
 * Section: Check  —  replaces <zephyr/sys/check.h>
 *
 * Zephyr's CHECKIF() macro for parameter validation.
 * ====================================================================== */
#ifndef BM_COMPAT_CHECK_H__
#define BM_COMPAT_CHECK_H__

#ifndef CHECKIF
#define CHECKIF(cond)  if (cond)
#endif

#endif /* BM_COMPAT_CHECK_H__ */


/* ======================================================================
 * Section: Reboot  —  replaces <zephyr/sys/reboot.h>
 * ====================================================================== */
#ifndef BM_COMPAT_REBOOT_H__
#define BM_COMPAT_REBOOT_H__

#define SYS_REBOOT_WARM  0
#define SYS_REBOOT_COLD  1

static inline void sys_reboot(int type)
{
	(void)type;
	NVIC_SystemReset();
}

#endif /* BM_COMPAT_REBOOT_H__ */


/* ======================================================================
 * Section: Iterable sections  —  replaces <zephyr/sys/iterable_sections.h>
 *
 * GCC linker section-based pattern for statically registered objects
 * with priority ordering.  Used by the SoftDevice handler observer
 * system but available for any module.
 *
 * TYPE_SECTION_ITERABLE places a struct in a named section with a
 * priority suffix for linker sorting.  TYPE_SECTION_FOREACH iterates
 * over all entries between the __start_ / __stop_ symbols that must
 * be defined in the linker script (see nrf_sdh_sections.ld).
 *
 * Two-level macro ensures the priority argument is expanded before
 * stringification (e.g. PRIO_LEVEL_ORD(USER) → 2 → "secname.2").
 * ====================================================================== */
#ifndef BM_COMPAT_ITERABLE_SECTIONS_H__
#define BM_COMPAT_ITERABLE_SECTIONS_H__

/**
 * Place a struct instance into a named linker section.
 *
 * @param _type    Struct type
 * @param _name    Variable name
 * @param _secname Linker section base name
 * @param _prio    Priority suffix for SORT ordering (must be a literal
 *                 or a macro that expands to one)
 */
#define TYPE_SECTION_ITERABLE(_type, _name, _secname, _prio) \
	_BM_SECTION_ITERABLE_(_type, _name, _secname, _prio)

#define _BM_SECTION_ITERABLE_(_type, _name, _secname, _prio) \
	_type _name \
	__attribute__((section(#_secname "." #_prio),            \
	               used, aligned(__alignof__(_type))))

/**
 * Iterate over all entries in a named linker section.
 *
 * @param _type    Struct type
 * @param _secname Linker section base name (must match TYPE_SECTION_ITERABLE)
 * @param _var     Iterator variable name (pointer to _type)
 *
 * Requires __start_<secname> / __stop_<secname> linker symbols.
 */
#define TYPE_SECTION_FOREACH(_type, _secname, _var)              \
	extern _type __start_##_secname[];                           \
	extern _type __stop_##_secname[];                            \
	for (_type *_var = __start_##_secname;                       \
	     _var < __stop_##_secname; _var++)

#endif /* BM_COMPAT_ITERABLE_SECTIONS_H__ */


/* ======================================================================
 * Section: Printk  —  replaces <zephyr/sys/printk.h>
 *
 * Already defined in Kernel section above, but some files include
 * printk.h directly.
 * ====================================================================== */
#ifndef BM_COMPAT_PRINTK_H__
#define BM_COMPAT_PRINTK_H__

/* printk already defined in Kernel section */

#endif /* BM_COMPAT_PRINTK_H__ */


/* ======================================================================
 * Section: Memory swap  —  replaces <zephyr/sys/byteorder.h> helpers
 *
 * sys_mem_swap:      in-place byte reversal
 * sys_memcpy_swap:   byte-reversed copy (src → dst)
 * Used by BLE LESC for ECC key endianness conversion.
 * ====================================================================== */
#ifndef BM_COMPAT_MEMSWAP_H__
#define BM_COMPAT_MEMSWAP_H__

static inline void sys_mem_swap(void *buf, size_t length)
{
	uint8_t *lo = (uint8_t *)buf;
	uint8_t *hi = lo + length - 1;

	while (lo < hi) {
		uint8_t tmp = *lo;
		*lo++ = *hi;
		*hi-- = tmp;
	}
}

static inline void sys_memcpy_swap(void *dst, const void *src, size_t length)
{
	const uint8_t *s = (const uint8_t *)src + length - 1;
	uint8_t *d = (uint8_t *)dst;

	for (size_t i = 0; i < length; i++) {
		*d++ = *s--;
	}
}

#endif /* BM_COMPAT_MEMSWAP_H__ */


/* ======================================================================
 * Section: LISTIFY  —  replaces <zephyr/sys/util_macro.h> LISTIFY
 *
 * LISTIFY(N, FN, sep, ...)  →  FN(0, ...) sep FN(1, ...) sep ... FN(N-1, ...)
 *
 * Supports up to 20 (sufficient for BLE_GQ_CUSTOM_DEF link counts).
 * ====================================================================== */
#ifndef BM_COMPAT_LISTIFY_H__
#define BM_COMPAT_LISTIFY_H__

#define _LISTIFY_0(F, s, ...)
#define _LISTIFY_1(F, s, ...)  F(0, ##__VA_ARGS__)
#define _LISTIFY_2(F, s, ...)  _LISTIFY_1(F, s, ##__VA_ARGS__) __DEBRACKET s F(1, ##__VA_ARGS__)
#define _LISTIFY_3(F, s, ...)  _LISTIFY_2(F, s, ##__VA_ARGS__) __DEBRACKET s F(2, ##__VA_ARGS__)
#define _LISTIFY_4(F, s, ...)  _LISTIFY_3(F, s, ##__VA_ARGS__) __DEBRACKET s F(3, ##__VA_ARGS__)
#define _LISTIFY_5(F, s, ...)  _LISTIFY_4(F, s, ##__VA_ARGS__) __DEBRACKET s F(4, ##__VA_ARGS__)
#define _LISTIFY_6(F, s, ...)  _LISTIFY_5(F, s, ##__VA_ARGS__) __DEBRACKET s F(5, ##__VA_ARGS__)
#define _LISTIFY_7(F, s, ...)  _LISTIFY_6(F, s, ##__VA_ARGS__) __DEBRACKET s F(6, ##__VA_ARGS__)
#define _LISTIFY_8(F, s, ...)  _LISTIFY_7(F, s, ##__VA_ARGS__) __DEBRACKET s F(7, ##__VA_ARGS__)
#define _LISTIFY_9(F, s, ...)  _LISTIFY_8(F, s, ##__VA_ARGS__) __DEBRACKET s F(8, ##__VA_ARGS__)
#define _LISTIFY_10(F, s, ...) _LISTIFY_9(F, s, ##__VA_ARGS__) __DEBRACKET s F(9, ##__VA_ARGS__)
#define _LISTIFY_11(F, s, ...) _LISTIFY_10(F, s, ##__VA_ARGS__) __DEBRACKET s F(10, ##__VA_ARGS__)
#define _LISTIFY_12(F, s, ...) _LISTIFY_11(F, s, ##__VA_ARGS__) __DEBRACKET s F(11, ##__VA_ARGS__)
#define _LISTIFY_13(F, s, ...) _LISTIFY_12(F, s, ##__VA_ARGS__) __DEBRACKET s F(12, ##__VA_ARGS__)
#define _LISTIFY_14(F, s, ...) _LISTIFY_13(F, s, ##__VA_ARGS__) __DEBRACKET s F(13, ##__VA_ARGS__)
#define _LISTIFY_15(F, s, ...) _LISTIFY_14(F, s, ##__VA_ARGS__) __DEBRACKET s F(14, ##__VA_ARGS__)
#define _LISTIFY_16(F, s, ...) _LISTIFY_15(F, s, ##__VA_ARGS__) __DEBRACKET s F(15, ##__VA_ARGS__)
#define _LISTIFY_17(F, s, ...) _LISTIFY_16(F, s, ##__VA_ARGS__) __DEBRACKET s F(16, ##__VA_ARGS__)
#define _LISTIFY_18(F, s, ...) _LISTIFY_17(F, s, ##__VA_ARGS__) __DEBRACKET s F(17, ##__VA_ARGS__)
#define _LISTIFY_19(F, s, ...) _LISTIFY_18(F, s, ##__VA_ARGS__) __DEBRACKET s F(18, ##__VA_ARGS__)
#define _LISTIFY_20(F, s, ...) _LISTIFY_19(F, s, ##__VA_ARGS__) __DEBRACKET s F(19, ##__VA_ARGS__)

#ifndef LISTIFY
#define LISTIFY(n, F, s, ...) CONCAT(_LISTIFY_, n)(F, s, ##__VA_ARGS__)
#endif

#endif /* BM_COMPAT_LISTIFY_H__ */


/* ======================================================================
 * Section: Tracing  —  replaces <zephyr/tracing/tracing.h>
 *
 * All SYS_PORT_TRACING_* macros are no-ops.
 * ====================================================================== */
#ifndef BM_COMPAT_TRACING_H__
#define BM_COMPAT_TRACING_H__

#ifndef SYS_PORT_TRACING_OBJ_INIT
#define SYS_PORT_TRACING_OBJ_INIT(...)           ((void)0)
#define SYS_PORT_TRACING_OBJ_FUNC_ENTER(...)     ((void)0)
#define SYS_PORT_TRACING_OBJ_FUNC_EXIT(...)      ((void)0)
#define SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(...)  ((void)0)
#endif

#endif /* BM_COMPAT_TRACING_H__ */


/* ======================================================================
 * Section: Types  —  replaces <zephyr/types.h>
 * ====================================================================== */
#ifndef BM_COMPAT_TYPES_H__
#define BM_COMPAT_TYPES_H__

/* Zephyr types.h just pulls in stdint + stddef, already included above */

#endif /* BM_COMPAT_TYPES_H__ */


/* Zephyr kernel data structures: sys_slist, k_mem_slab, k_heap */
#include "bm_compat_ds.h"

/* SWI handlers used by SoftDevice S145 (defined as weak in startup .S) */
void SWI01_IRQHandler(void);
void SWI02_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* BM_COMPAT_H__ */
