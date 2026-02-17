/**
 * @brief nrf_strerror — Convert NRF_ERROR_* codes to human-readable strings.
 *
 * Minimal replacement for the nRF5 SDK nrf_strerror module.
 */
#ifndef NRF_STRERROR_H__
#define NRF_STRERROR_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Look up the error description for an nrf_error code.
 * @param code  An NRF_ERROR_* value.
 * @return Pointer to a constant descriptive string, or a generic
 *         "unknown" string if the code is not recognised.
 */
const char *nrf_strerror_get(uint32_t code);

/**
 * @brief Same as nrf_strerror_get but returns NULL when not found.
 */
const char *nrf_strerror_find(uint32_t code);

#ifdef __cplusplus
}
#endif

#endif /* NRF_STRERROR_H__ */
