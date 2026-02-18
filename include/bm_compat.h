/**-------------------------------------------------------------------------
@file	bm_compat.h

@brief	Zephyr API compatibility layer for sdk-nrf-bm bare-metal conversion

Drop-in replacements for Zephyr APIs used by sdk-nrf-bm.

Target: sdk-nrf-bm v2.9.1 (nRF Connect SDK bare-metal)
Platform: Cortex-M33 (nRF54L series)

Usage:
  Replace all <zephyr/...> includes with:
    #include "bm_compat.h"

@author	IOsonata
@date	2025

@license MIT
----------------------------------------------------------------------------*/

#ifndef BM_COMPAT_H__
#define BM_COMPAT_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

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
 * Logging  —  replaces <zephyr/logging/log.h>
 * No-ops.  162 call sites in sdk-nrf-bm.
 * ====================================================================== */

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

#define LOG_PANIC()     ((void)0)
#define LOG_INIT()      ((void)0)
#define log_flush()     ((void)0)


/* ======================================================================
 * Util  —  replaces <zephyr/sys/util.h>, util_macro.h
 * ====================================================================== */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, align)  (((x) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef BYTES_TO_WORDS
#define BYTES_TO_WORDS(bytes)  (((bytes) + 3) / 4)
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

#ifndef IN_RANGE
#define IN_RANGE(val, min, max)  ((val) >= (min) && (val) <= (max))
#endif

#ifndef IS_ALIGNED
#define IS_ALIGNED(x, align)  (((x) & ((align) - 1)) == 0)
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#ifndef SIZEOF_FIELD
#define SIZEOF_FIELD(type, member)  sizeof(((type *)0)->member)
#endif

#ifndef CONCAT
#define _CONCAT2(a, b)  a ## b
#define CONCAT(a, b)    _CONCAT2(a, b)
#endif

#ifndef USEC_PER_SEC
#define USEC_PER_SEC   1000000UL
#endif

#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC  1000UL
#endif

/**
 * IS_ENABLED — test boolean Kconfig options.
 * Returns 1 if macro is defined to 1, 0 otherwise.
 */
#ifndef IS_ENABLED
#define _ZZZZ1                            _ZIGN,
#define _IS_ENABLED_3(_ign, val, ...)     val
#define _IS_ENABLED_2(one_or_two)         _IS_ENABLED_3(one_or_two 1, 0)
#define _IS_ENABLED_1(x)                  _IS_ENABLED_2(_ZZZZ ## x)
#define IS_ENABLED(config_macro)          _IS_ENABLED_1(config_macro)
#endif

/**
 * COND_CODE_1 — conditional expansion based on macro value.
 * If flag == 1, expand if1 (parenthesized); else expand el.
 */
#ifndef COND_CODE_1
#define _XXXX1 _YYYY,
#define __DEBRACKET(...)                __VA_ARGS__
#define __GET_ARG2_DEBRACKET(ign, val, ...)  __DEBRACKET val
#define __COND_CODE(one_or_two, if1, el)     __GET_ARG2_DEBRACKET(one_or_two if1, el)
#define Z_COND_CODE_1(flag, if1, el)         __COND_CODE(_XXXX##flag, if1, el)
#define COND_CODE_1(flag, if1, el)           Z_COND_CODE_1(flag, if1, el)
#endif

/**
 * IS_EMPTY — expands to 1 when __VA_ARGS__ is empty, 0 otherwise.
 * Jens Gustedt technique.  Used by dis.c gatt_char() macro.
 */
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
#endif


/* ======================================================================
 * Assert  —  replaces <zephyr/sys/__assert.h>
 * ====================================================================== */

#ifndef __ASSERT
#define __ASSERT(cond, ...)  assert(cond)
#endif

#ifndef __ASSERT_NO_MSG
#define __ASSERT_NO_MSG(cond)  assert(cond)
#endif

#ifndef BUILD_ASSERT
#define BUILD_ASSERT(cond, ...)  _Static_assert(cond, "" __VA_ARGS__)
#endif


/* ======================================================================
 * Toolchain  —  replaces <zephyr/toolchain.h>
 * ====================================================================== */

#ifndef __weak
#define __weak  __attribute__((weak))
#endif

#ifndef __packed
#define __packed  __attribute__((packed))
#endif

#ifndef __aligned
#define __aligned(x)  __attribute__((aligned(x)))
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

#ifndef ARG_UNUSED
#define ARG_UNUSED(x)  (void)(x)
#endif

#ifndef STRINGIFY
#define _STRINGIFY(x)  #x
#define STRINGIFY(x)   _STRINGIFY(x)
#endif


/* ======================================================================
 * IRQ  —  replaces <zephyr/irq.h>
 * Maps to CMSIS NVIC.
 * ====================================================================== */

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

#ifndef ISR_DIRECT_DECLARE
#define ISR_DIRECT_DECLARE(name) static int name(void)
#endif

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


/* ======================================================================
 * Kernel  —  replaces <zephyr/kernel.h> primitives
 * ====================================================================== */

#ifdef __arm__
#define k_cpu_idle()  __WFE()
#else
#define k_cpu_idle()  do {} while (0)
#endif

/* Tick frequency and conversion macros — replaces <zephyr/sys/time_units.h> */
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

#define k_ticks_to_ms_floor32(ticks) \
	((uint32_t)(((uint64_t)(ticks) * 1000U) / CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define k_ticks_to_us_ceil32(us) \
	((uint32_t)(((uint64_t)(us) * 1000000U + CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) / CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/**
 * k_uptime_ticks — monotonic tick count via GRTC SYSCOUNTER (16 MHz).
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

/**
 * k_busy_wait — spin for N microseconds.
 */
static inline void k_busy_wait(uint32_t usec)
{
#if defined(DWT) && defined(CoreDebug)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	uint32_t cycles = (SystemCoreClock / 1000000UL) * usec;
	while (DWT->CYCCNT < cycles) {}
#else
	volatile uint32_t count = (SystemCoreClock / 4000000UL) * usec;
	while (count--) { __NOP(); }
#endif
}

#ifndef printk
#define printk(...)  ((void)0)
#endif


/* ======================================================================
 * Byteorder  —  replaces <zephyr/sys/byteorder.h>
 * ARM Cortex-M is little-endian.
 * ====================================================================== */

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

/* LE identity */
static inline uint16_t sys_le16_to_cpu(uint16_t val) { return val; }
static inline uint32_t sys_le32_to_cpu(uint32_t val) { return val; }
static inline uint16_t sys_cpu_to_le16(uint16_t val) { return val; }
static inline uint32_t sys_cpu_to_le32(uint32_t val) { return val; }

/* BE swap */
static inline uint16_t sys_be16_to_cpu(uint16_t val) { return __builtin_bswap16(val); }
static inline uint32_t sys_be32_to_cpu(uint32_t val) { return __builtin_bswap32(val); }
static inline uint16_t sys_cpu_to_be16(uint16_t val) { return __builtin_bswap16(val); }
static inline uint32_t sys_cpu_to_be32(uint32_t val) { return __builtin_bswap32(val); }

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


/* ======================================================================
 * Atomic  —  replaces <zephyr/sys/atomic.h>
 * ====================================================================== */
#ifndef BM_ATOMIC_H__
#include "bm_atomic.h"
#endif


/* ======================================================================
 * Init  —  replaces <zephyr/init.h>
 * SYS_INIT() stripped — call init functions explicitly.
 * ====================================================================== */

#ifndef SYS_INIT
#define SYS_INIT(fn, level, prio)
#endif


/* ======================================================================
 * Singly-linked list  —  replaces <zephyr/sys/slist.h>
 * ====================================================================== */

struct _snode {
	struct _snode *next;
};

typedef struct _snode sys_snode_t;

struct _slist {
	sys_snode_t *head;
	sys_snode_t *tail;
};

typedef struct _slist sys_slist_t;

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


/* ======================================================================
 * Memory swap  —  replaces <zephyr/sys/byteorder.h> helpers
 * Used by BLE LESC for ECC key endianness conversion.
 * ====================================================================== */

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


/* ======================================================================
 * CRC  —  replaces <zephyr/sys/crc.h>
 * ====================================================================== */

#include "crc.h"


/* ======================================================================
 * LISTIFY  —  replaces <zephyr/sys/util_macro.h> LISTIFY
 * LISTIFY(N, FN, sep, ...) → FN(0,...) sep FN(1,...) ... FN(N-1,...)
 * Supports up to 10 (sufficient for BLE link counts).
 * ====================================================================== */

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

#ifndef LISTIFY
#define LISTIFY(n, F, s, ...) CONCAT(_LISTIFY_, n)(F, s, ##__VA_ARGS__)
#endif


/* ======================================================================
 * Iterable sections  —  replaces <zephyr/sys/iterable_sections.h>
 *
 * GCC linker section-based pattern for statically registered objects.
 * Used by SoftDevice handler observer system.
 * ====================================================================== */

#define TYPE_SECTION_ITERABLE(_type, _name, _secname, _prio) \
	_BM_SECTION_ITERABLE_(_type, _name, _secname, _prio)

#define _BM_SECTION_ITERABLE_(_type, _name, _secname, _prio) \
	_type _name \
	__attribute__((section(#_secname "." #_prio),            \
	               used, aligned(__alignof__(_type))))

#define TYPE_SECTION_FOREACH(_type, _secname, _var)              \
	extern _type __start_##_secname[];                           \
	extern _type __stop_##_secname[];                            \
	for (_type *_var = __start_##_secname;                       \
	     _var < __stop_##_secname; _var++)


/* ======================================================================
 * CHECKIF  —  replaces <zephyr/sys/check.h>
 * ====================================================================== */

#ifndef CHECKIF
#define CHECKIF(cond)  if (cond)
#endif


/* Zephyr kernel data structures: k_mem_slab, k_heap, ring_buf */
#include "bm_compat_ds.h"

/* SWI handlers used by SoftDevice S145 */
void SWI01_IRQHandler(void);
void SWI02_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* BM_COMPAT_H__ */
