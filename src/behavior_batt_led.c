/*
 * Keymap behavior `&batt_led_show`: on press, show the current battery level
 * on the indicator LED. Declared with GLOBAL locality so ZMK invokes it on
 * every split half, not just the central — each half then blinks its own LED
 * with its own battery level. The actual work lives in battery_led.c; the weak
 * stub below is a safety net so this still links on a half with no LED wired.
 */

#define DT_DRV_COMPAT zmk_behavior_batt_led

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>

LOG_MODULE_REGISTER(behavior_batt_led, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

__weak void batt_led_show(void) {}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    batt_led_show();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_batt_led_driver_api = {
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

static int behavior_batt_led_init(const struct device *dev) {
    return 0;
}

BEHAVIOR_DT_INST_DEFINE(0, behavior_batt_led_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_batt_led_driver_api);

#endif
