# sdk-nrf-bm Zephyr Removal — Conversion Manifest

## Summary

- **123 source files** converted (all `#include <zephyr/...>` removed)
- **0 remaining** `<zephyr/>` includes in `.c`/`.h` files
- **4 new headers** added to `include/bm/`

---

## New Headers

### `bm_compat.h` — Main compatibility layer
Sections (each independently overridable via include guard):

| Section | Replaces | Status |
|---------|----------|--------|
| Logging | `<zephyr/logging/log.h>`, `log_ctrl.h` | No-ops |
| Util | `<zephyr/sys/util.h>`, `util_macro.h` | Full (ARRAY_SIZE, BIT, IS_ENABLED, COND_CODE_1, SIZEOF_FIELD) |
| Assert | `<zephyr/sys/__assert.h>` | Maps to C11 assert/static_assert |
| Toolchain | `<zephyr/toolchain.h>` | GCC attributes (__weak, __packed, __aligned, __ALIGN) |
| IRQ | `<zephyr/irq.h>` | CMSIS NVIC (irq_lock/unlock, ISR_DIRECT_DECLARE, IRQ_DIRECT_CONNECT) |
| Kernel | `<zephyr/kernel.h>` | k_cpu_idle→WFE, k_uptime_ticks→GRTC, k_busy_wait→DWT |
| Byteorder | `<zephyr/sys/byteorder.h>` | LE/BE get/put 16/32, sys_cpu_to_* |
| Atomic | `<zephyr/sys/atomic.h>` | → bm_atomic.h (C11 stdatomic) |
| Init | `<zephyr/init.h>` | SYS_INIT() stripped |
| Slist | `<zephyr/sys/slist.h>` | Full intrusive singly-linked list |
| Barrier | `<zephyr/sys/barrier.h>` | DMB/ISB |
| CRC | `<zephyr/sys/crc.h>` | → IOsonata crc.h |
| Check | `<zephyr/sys/check.h>` | CHECKIF() |
| Reboot | `<zephyr/sys/reboot.h>` | NVIC_SystemReset() |
| Iterable sections | `<zephyr/sys/iterable_sections.h>` | GCC section attributes |
| Printk | `<zephyr/sys/printk.h>` | No-op |
| **Memory swap** | `<zephyr/sys/byteorder.h>` helpers | **NEW** — sys_mem_swap, sys_memcpy_swap |
| **LISTIFY** | `<zephyr/sys/util_macro.h>` | **NEW** — Supports 0–20, used by BLE_GQ |
| **Tracing** | `<zephyr/tracing/tracing.h>` | **NEW** — SYS_PORT_TRACING_* no-ops |
| **Types** | `<zephyr/types.h>` | **NEW** — stdint/stddef already included |

### `bm_compat_ds.h` — Data structures
- k_mem_slab (fixed-size block allocator)
- k_heap (first-fit variable-size allocator)
- **Ring buffer** (**NEW** — RING_BUF_DECLARE, ring_buf_put/get/is_empty/space_get/reset)

### `bm_atomic.h` — C11 stdatomic wrappers
- atomic_t, atomic_val_t, atomic_ptr_t
- All integer atomics (set/get/inc/dec/add/sub/or/and/xor/cas)
- All bit operations (set_bit/clear_bit/test_bit/test_and_set_bit/test_and_clear_bit)
- Pointer atomics (atomic_ptr_cas/get/set)
- ATOMIC_INIT, ATOMIC_DEFINE, ATOMIC_BITMAP_SIZE

### `bm_config_defaults.h` — Kconfig defaults
All numeric/string CONFIG_ defaults extracted from Kconfig files, organized by subsystem.
Every value guarded with `#ifndef` — override by defining before include.

---

## CRC Parameter Order — ACTION REQUIRED

Zephyr convention: `crc8_ccitt(seed, data, len)`
IOsonata convention: `crc8_ccitt(data, len, seed)`

Call sites in `bm_zms.c`, `bm_installs.c`, and `installer/main.c` use Zephyr order.
Either:
1. Add wrapper that reorders params in bm_compat.h, or
2. Swap params at each call site

`crc32_ieee(data, len)` has no seed — compatible as-is.

---

## Known Remaining Zephyr Dependencies (Tier 3/4 — includes removed, logic intact)

### MCUmgr subsystem
- `lib/zephyr_queue/src/queue.c` — needs `sys_sflist`, `wait_q.h`, `ksched.h`
- `subsys/mgmt/mcumgr/transport/` — `net_buf`, SMP transport
- `subsys/bluetooth/services/ble_mcumgr/` — net_buf BLE transport

### Flash/Storage
- `drivers/flash/soc_flash_nrf_rram.c` — Zephyr flash device model
- `subsys/storage/flash_map/` — flash_map API

### Settings
- `subsys/settings/` — Zephyr settings subsystem

### Log backends
- `subsys/logging/backends/log_backend_bm_uarte.c` — log backend API

### Boot/Retention
- `samples/boot/` — MCUboot retention, bootmode APIs
- `subsys/nfc/` — NFC platform layer

### PSA Crypto
- `lib/bluetooth/peer_manager/modules/nrf_ble_lesc.c` — `<psa/crypto.h>`
  (real dependency, not Zephyr — needs sdk-oberon-psa-crypto include path)

---

## Files Converted by Category

### BLE Services (Tier 1) — 14 files
subsys/bluetooth/services/ble_bas/, ble_bms/, ble_cgms/ (×6), ble_dis/,
ble_hids/, ble_hrs/, ble_hrs_client/, ble_lbs/, ble_mcumgr/, ble_nus/

### BLE Libraries (Tier 1) — 18 files
lib/bluetooth/ble_adv/ (×2), ble_conn_params/ (×5), ble_conn_state/,
ble_db_discovery/, ble_gq/, ble_qwr/, ble_radio_notification/, ble_scan/

### Peer Manager (Tier 1/2) — 12 files
All modules: auth_status_tracker, gatt_cache_manager, gatts_cache_manager,
id_manager, nrf_ble_lesc, peer_data_storage, peer_database, peer_id,
peer_manager_handler, pm_buffer, security_dispatcher, security_manager, peer_manager

### SoftDevice Handler (Tier 1) — 5 files
nrf_sdh.c, nrf_sdh_ble.c, nrf_sdh_soc.c, irq_connect.c, rand_seed.c

### Drivers (Tier 1/2) — 3 files
lpuarte.c, console_bm_uarte.c, soc_flash_nrf_rram.c

### Core Libraries (Tier 1) — 5 files
bm_scheduler, bm_timer, bm_buttons, bm_gpiote, boot_banner

### Subsystems (Tier 1/2) — 7 files
bm_zms, bm_storage (×4), bm_installs, zephyr_queue

### Samples (Tier 1) — 18 files
All BLE samples, peripheral samples, NFC samples, subsys samples

### Tests (Tier 1) — 10 files
Unit tests for BLE modules

### Includes (Tier 1) — ~20 header files
All public headers in include/bm/
