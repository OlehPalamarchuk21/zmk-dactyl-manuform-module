/*
 * Single green battery-level indicator LED.
 *
 * Compiled on each split half that defines a `batt-led` gpio-leds node (both
 * halves here). Each LED encodes its own half's battery state of charge:
 *   - 3 blinks = good, 2 = medium, 1 = low (see thresholds in Kconfig),
 *   - a single repeating blink every 30s when at/below the critical level
 *     (default 10%), as a low-battery heartbeat.
 *
 * The level is announced once after boot (the first battery reading) and on
 * demand via the `&batt_led_show` keymap behavior (GLOBAL locality, so one
 * keypress drives both halves' LEDs). Between blinks the LED is dark, so idle
 * draw is essentially zero.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

LOG_MODULE_REGISTER(batt_led, CONFIG_ZMK_LOG_LEVEL);

#if !DT_NODE_EXISTS(DT_ALIAS(batt_led))
#error "Define a `batt-led` alias pointing to a gpio-leds child node"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(batt_led), gpios);

#define BLINK_MS       CONFIG_DACTYL_BATT_LED_BLINK_MS
#define HIGH_PCT       CONFIG_DACTYL_BATT_LED_LEVEL_HIGH
#define LOW_PCT        CONFIG_DACTYL_BATT_LED_LEVEL_LOW
#define CRIT_PCT       CONFIG_DACTYL_BATT_LED_LEVEL_CRITICAL
#define CRIT_INTERVAL  CONFIG_DACTYL_BATT_LED_CRITICAL_INTERVAL_SEC

// Last reported state of charge. Defaults to a healthy value so an on-demand
// request before the first battery reading does not falsely trigger the
// critical heartbeat.
static uint8_t last_soc = 100;
static bool have_reading;

// True while the periodic critical-battery heartbeat is running.
static bool led_critical;

// Remaining on/off half-steps in the current blink sequence (two per blink).
static uint8_t half_steps_remaining;
static bool led_on;

static void blink_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(blink_work, blink_work_handler);

static void critical_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(critical_work, critical_work_handler);

#if IS_ENABLED(CONFIG_DACTYL_BATT_LED_BOOT_SELFTEST)
static void selftest_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(selftest_work, selftest_work_handler);
#endif

static void blink_work_handler(struct k_work *work) {
    if (half_steps_remaining == 0) {
        gpio_pin_set_dt(&led, 0);
        led_on = false;
        return;
    }

    led_on = !led_on;
    gpio_pin_set_dt(&led, led_on ? 1 : 0);
    half_steps_remaining--;

    k_work_schedule(&blink_work, K_MSEC(BLINK_MS));
}

static void blink(uint8_t count) {
    half_steps_remaining = count * 2;
    led_on = false;
    k_work_reschedule(&blink_work, K_NO_WAIT);
}

static uint8_t blinks_for(uint8_t soc) {
    if (soc > HIGH_PCT) {
        return 3;
    }
    if (soc > LOW_PCT) {
        return 2;
    }
    return 1;
}

// Critical-battery heartbeat: one blink, then re-arm for the next interval.
static void critical_work_handler(struct k_work *work) {
    blink(1);
    k_work_schedule(&critical_work, K_SECONDS(CRIT_INTERVAL));
}

#if IS_ENABLED(CONFIG_DACTYL_BATT_LED_BOOT_SELFTEST)
// Diagnostic: blink twice, then re-arm, forever. Ignores the battery entirely
// so it proves only the GPIO -> resistor -> LED path on this half.
static void selftest_work_handler(struct k_work *work) {
    blink(2);
    k_work_schedule(&selftest_work,
                    K_SECONDS(CONFIG_DACTYL_BATT_LED_BOOT_SELFTEST_INTERVAL_SEC));
}
#endif

// Reflect the current battery level on the LED. When announce is true a blink
// sequence is shown; the critical heartbeat is started (or stopped) regardless,
// since it must engage the moment the battery crosses the line.
static void apply_state(bool announce) {
    if (last_soc <= CRIT_PCT) {
        if (!led_critical) {
            led_critical = true;
            k_work_reschedule(&critical_work, K_NO_WAIT);
        }
        return;
    }

    if (led_critical) {
        led_critical = false;
        k_work_cancel_delayable(&critical_work);
        k_work_cancel_delayable(&blink_work);
        half_steps_remaining = 0;
        gpio_pin_set_dt(&led, 0);
    }

    if (announce) {
        blink(blinks_for(last_soc));
    }
}

// Called from the &batt_led_show keymap behavior. Overrides the weak no-op in
// behavior_batt_led.c on the half that actually carries the LED.
void batt_led_show(void) {
    blink(blinks_for(last_soc));
}

static int batt_led_listener(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return 0;
    }

    last_soc = ev->state_of_charge;
    LOG_DBG("battery %u%%", last_soc);

    apply_state(!have_reading);
    have_reading = true;
    return 0;
}

ZMK_LISTENER(batt_led, batt_led_listener);
ZMK_SUBSCRIPTION(batt_led, zmk_battery_state_changed);

static int batt_led_init(void) {
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("battery LED GPIO not ready");
        return -ENODEV;
    }
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

#if IS_ENABLED(CONFIG_DACTYL_BATT_LED_BOOT_SELFTEST)
    LOG_WRN("battery LED boot self-test active");
    k_work_schedule(&selftest_work, K_SECONDS(2));
#endif

    return ret;
}

SYS_INIT(batt_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
