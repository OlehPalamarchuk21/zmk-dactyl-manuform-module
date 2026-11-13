/*
 * Low-voltage cutoff: put the board into ZMK soft-off when the cell runs low.
 *
 * The cells on this keyboard have no protection PCB and the Supermini has no
 * over-discharge protection, so without this the board keeps running (and
 * draining) far below 0% until the nRF52840 browns out - and the cell keeps
 * draining after that. Soft-off stops the drain at a safe voltage while the
 * cell still has a margin above its 2.5 V limit.
 *
 * Every sample of the battery sensor ends up in dactyl_battery_cutoff_check().
 * ZMK samples only at boot and while the keyboard is active, so after the first
 * low sample this module takes the confirmation samples itself, a few seconds
 * apart. All samples run on ZMK's low-priority work queue, so they never
 * overlap.
 *
 * zmk_pm_soft_off() turns wakeup off on every device, including kscan0, so a
 * key press cannot wake the board (no reboot loop on an empty cell). Plugging
 * in USB or pressing reset wakes it.
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

#include <zmk/pm.h>
#include <zmk/workqueue.h>

// Same USB check as ZMK's activity.c. It also works on the right half, which
// has USB_DEVICE_STACK but not ZMK_USB (that is why battery_ncr18650.c, which
// only needs it on the central, checks CONFIG_ZMK_USB instead).
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/usb.h>
#endif

#include "battery_cutoff.h"
#include "battery_led.h"

LOG_MODULE_REGISTER(batt_cutoff, CONFIG_ZMK_LOG_LEVEL);

#define CONFIRM_INTERVAL_SEC 5

static const struct device *const battery = DEVICE_DT_GET_ONE(dactyl_battery_ncr18650);

// Only used on the low-priority work queue, where every sensor sample runs.
static uint8_t low_samples;

static bool usb_powered(void) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    return zmk_usb_is_powered();
#else
    return false;
#endif
}

static void confirm_work_handler(struct k_work *work) {
    // The sensor driver passes the new sample to dactyl_battery_cutoff_check().
    const int rc = sensor_sample_fetch(battery);
    if (rc != 0) {
        // Keep the confirmation going: ZMK itself does not sample while idle.
        LOG_WRN("Confirmation sample failed: %d", rc);
        k_work_schedule_for_queue(zmk_workqueue_lowprio_work_q(),
                                  k_work_delayable_from_work(work),
                                  K_SECONDS(CONFIRM_INTERVAL_SEC));
    }
}

static K_WORK_DELAYABLE_DEFINE(confirm_work, confirm_work_handler);

// Runs on the system work queue, like the battery LED work items and the deep
// sleep in ZMK's activity.c. The LED can only be switched on from this queue,
// and this handler never returns (except when USB came back, see below), so the
// LED stays off through System OFF.
static void cutoff_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    // USB plugged in after the last low sample: do not power off. With USB
    // already present, plugging it in could not wake the board. One more
    // sample (on USB) resets the count.
    if (usb_powered()) {
        LOG_INF("USB powered: cutoff cancelled");
        k_work_schedule_for_queue(zmk_workqueue_lowprio_work_q(), &confirm_work, K_NO_WAIT);
        return;
    }

    LOG_WRN("Cell below %d mV: entering soft-off", CONFIG_DACTYL_BATTERY_CUTOFF_MV);

    // nRF52 GPIO outputs keep their level in System OFF: an LED caught in the
    // middle of a blink would stay lit and drain the cell.
    batt_led_force_off();

    const int rc = zmk_pm_soft_off();

    // zmk_pm_soft_off() only returns on failure, after it has already suspended
    // kscan and most other devices, so the board is no longer usable. Power off
    // anyway: stopping the drain is the point.
    LOG_ERR("Soft-off failed (%d), powering off directly", rc);
    sys_poweroff();
}

static K_WORK_DEFINE(cutoff_work, cutoff_work_handler);

void dactyl_battery_cutoff_check(uint16_t millivolts) {
    // On USB the sample is the USB rail, not the cell, and the cell is charging.
    if (usb_powered() || millivolts >= CONFIG_DACTYL_BATTERY_CUTOFF_MV) {
        if (low_samples > 0) {
            LOG_DBG("Cutoff count reset at %u mV", millivolts);
        }
        low_samples = 0;
        k_work_cancel_delayable(&confirm_work);
        return;
    }

    if (low_samples < CONFIG_DACTYL_BATTERY_CUTOFF_SAMPLES) {
        low_samples++;
    }

    LOG_DBG("Low sample %u/%d: %u mV", low_samples, CONFIG_DACTYL_BATTERY_CUTOFF_SAMPLES,
            millivolts);

    if (low_samples >= CONFIG_DACTYL_BATTERY_CUTOFF_SAMPLES) {
        k_work_cancel_delayable(&confirm_work);
        // The ADC read for this sample is already complete, so soft-off may
        // suspend the ADC at once.
        k_work_submit(&cutoff_work);
        return;
    }

    // Keeps an earlier schedule if one is pending.
    k_work_schedule_for_queue(zmk_workqueue_lowprio_work_q(), &confirm_work,
                              K_SECONDS(CONFIRM_INTERVAL_SEC));
}
