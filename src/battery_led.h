#pragma once

/*
 * Battery LED functions used outside battery_led.c.
 *
 * battery_led.c defines them on a half with the LED (CONFIG_DACTYL_BATT_LED).
 * behavior_batt_led.c has weak no-op versions, so callers still link on a half
 * without it.
 */

// Show the battery level as 1-3 blinks (the &batt_led_show key).
void batt_led_show(void);

// Stop every blink and switch the LED off, right before the board powers off.
// Must be called on the system work queue.
void batt_led_force_off(void);
