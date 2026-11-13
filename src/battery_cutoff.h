#pragma once

#include <stdint.h>

#include <zephyr/sys/util_macro.h>

/*
 * Low-voltage cutoff for the bare (unprotected) 18650 cell.
 *
 * Called by the battery sensor after every successful VDDH sample, on ZMK's
 * low-priority work queue. After CONFIG_DACTYL_BATTERY_CUTOFF_SAMPLES samples
 * in a row below CONFIG_DACTYL_BATTERY_CUTOFF_MV on battery power, the board
 * goes to ZMK soft-off. Only USB power or the reset button wakes it again.
 */
#if IS_ENABLED(CONFIG_DACTYL_BATTERY_CUTOFF)
void dactyl_battery_cutoff_check(uint16_t millivolts);
#else
static inline void dactyl_battery_cutoff_check(uint16_t millivolts) { (void)millivolts; }
#endif
