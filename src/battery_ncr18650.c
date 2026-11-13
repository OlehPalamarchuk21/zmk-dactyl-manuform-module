/*
 * Battery sensor for Panasonic NCR18650B cells on a Supermini nRF52840.
 *
 * Samples the VDDH rail exactly the way ZMK's `zmk,battery-nrf-vddh` driver does
 * (SAADC VDDHDIV5 x 5), but differs from it in two ways that matter on this
 * keyboard:
 *
 *   1. Voltage -> state of charge goes through a real NCR18650B discharge curve
 *      instead of ZMK's single straight line (`mv * 2 / 15 - 459`, 3450 mV = 0%,
 *      4200 mV = 100%). That line spends 35 of its 100 points on the first ~10%
 *      of the cell's capacity and only ~25 points on the long 3.9-3.7 V plateau
 *      where most of the capacity lives, so the reported level collapses in the
 *      first days, sits still for a month, then falls off a cliff.
 *
 *   2. While USB is powered the sample is the USB rail, not the cell - VDDH is
 *      fed through the input diode and reads well above 4.2 V, so any mapping
 *      would report 100%. The last cell-derived level is held instead, and a
 *      real reading resumes on the first sample after unplugging.
 *
 * Wired up via `chosen { zmk,battery = &ncr_batt; }` in the shield dtsi.
 */

#define DT_DRV_COMPAT dactyl_battery_ncr18650

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_ZMK_USB)
#include <zmk/usb.h>
#endif

#include "battery_cutoff.h"

LOG_MODULE_REGISTER(batt_ncr, CONFIG_ZMK_LOG_LEVEL);

#define VDDHDIV (5)

static const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));

// Open-circuit discharge curve of an NCR18650B at the few-mA load a keyboard
// puts on it, in descending voltage order. Interpolated linearly between points.
//
// 0% sits at 3.20 V rather than the cell's 2.5 V cutoff: these are bare cells
// with no protection PCB, so the firmware calls empty while a safe margin is
// left. Recalibrate by logging `vddh ... mV -> ...%` (LOG_DBG below) across a
// real discharge and editing the points.
struct batt_curve_point {
    uint16_t millivolts;
    uint8_t percent;
};

static const struct batt_curve_point ncr18650b_curve[] = {
    {4200, 100}, {4150, 95}, {4100, 90}, {4050, 85}, {4000, 80}, {3950, 74}, {3900, 68},
    {3850, 62},  {3800, 55}, {3750, 48}, {3700, 40}, {3650, 33}, {3600, 26}, {3550, 20},
    {3500, 15},  {3450, 11}, {3400, 8},  {3300, 4},  {3200, 0},
};

static uint8_t ncr18650b_mv_to_pct(uint16_t bat_mv) {
    if (bat_mv >= ncr18650b_curve[0].millivolts) {
        return ncr18650b_curve[0].percent;
    }

    for (size_t i = 1; i < ARRAY_SIZE(ncr18650b_curve); i++) {
        const struct batt_curve_point *lower = &ncr18650b_curve[i];

        if (bat_mv >= lower->millivolts) {
            const struct batt_curve_point *upper = &ncr18650b_curve[i - 1];
            const uint32_t span_mv = upper->millivolts - lower->millivolts;
            const uint32_t span_pct = upper->percent - lower->percent;

            return lower->percent + ((uint32_t)(bat_mv - lower->millivolts) * span_pct) / span_mv;
        }
    }

    return 0;
}

struct ncr_data {
    struct adc_channel_cfg acc;
    struct adc_sequence as;
    uint16_t adc_raw;
    uint16_t millivolts;
    uint8_t state_of_charge;
    // Last level derived from a sample taken on battery power, i.e. one that
    // actually saw the cell. Held while USB feeds the rail.
    uint8_t cell_state_of_charge;
    bool have_cell_reading;
};

static int ncr_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    if (chan != SENSOR_CHAN_GAUGE_VOLTAGE && chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE &&
        chan != SENSOR_CHAN_ALL) {
        LOG_DBG("Selected channel is not supported: %d.", chan);
        return -ENOTSUP;
    }

    struct ncr_data *drv_data = dev->data;
    struct adc_sequence *as = &drv_data->as;

    int rc = adc_read(adc, as);
    as->calibrate = false;

    if (rc != 0) {
        LOG_ERR("Failed to read ADC: %d", rc);
        return rc;
    }

    int32_t val = drv_data->adc_raw;
    rc = adc_raw_to_millivolts(adc_ref_internal(adc), drv_data->acc.gain, as->resolution, &val);
    if (rc != 0) {
        LOG_ERR("Failed to convert raw ADC to mV: %d", rc);
        return rc;
    }

    drv_data->millivolts = val * VDDHDIV;

    const uint8_t measured_pct = ncr18650b_mv_to_pct(drv_data->millivolts);
    bool usb_powered = false;

#if IS_ENABLED(CONFIG_DACTYL_BATTERY_HOLD_WHILE_CHARGING) && IS_ENABLED(CONFIG_ZMK_USB)
    usb_powered = zmk_usb_is_powered();
#endif

    // With no cell reading yet (booted on USB) there is nothing to hold, so the
    // rail-derived value is reported until the first sample on battery power -
    // but it is never latched as if it came from the cell.
    const bool held = usb_powered && drv_data->have_cell_reading;

    if (held) {
        drv_data->state_of_charge = drv_data->cell_state_of_charge;
    } else {
        drv_data->state_of_charge = measured_pct;

        if (!usb_powered) {
            drv_data->cell_state_of_charge = measured_pct;
            drv_data->have_cell_reading = true;
        }
    }

    LOG_DBG("vddh %d mV -> %u%%%s", drv_data->millivolts, drv_data->state_of_charge,
            held ? " (held, USB powered)" : "");

    dactyl_battery_cutoff_check(drv_data->millivolts);

    return 0;
}

static int ncr_channel_get(const struct device *dev, enum sensor_channel chan,
                           struct sensor_value *val) {
    const struct ncr_data *drv_data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_GAUGE_VOLTAGE:
        val->val1 = drv_data->millivolts / 1000;
        val->val2 = (drv_data->millivolts % 1000) * 1000U;
        break;

    case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
        val->val1 = drv_data->state_of_charge;
        val->val2 = 0;
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

static const struct sensor_driver_api ncr_api = {
    .sample_fetch = ncr_sample_fetch,
    .channel_get = ncr_channel_get,
};

static int ncr_init(const struct device *dev) {
    struct ncr_data *drv_data = dev->data;

    if (!device_is_ready(adc)) {
        LOG_ERR("ADC device is not ready %s", adc->name);
        return -ENODEV;
    }

    drv_data->as = (struct adc_sequence){
        .channels = BIT(0),
        .buffer = &drv_data->adc_raw,
        .buffer_size = sizeof(drv_data->adc_raw),
        .oversampling = 4,
        .calibrate = true,
    };

#ifdef CONFIG_ADC_NRFX_SAADC
    drv_data->acc = (struct adc_channel_cfg){
        .gain = ADC_GAIN_1_2,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
        .input_positive = SAADC_CH_PSELN_PSELN_VDDHDIV5,
    };

    drv_data->as.resolution = 12;
#else
#error Unsupported ADC
#endif

    const int rc = adc_channel_setup(adc, &drv_data->acc);
    LOG_DBG("VDDHDIV5 setup returned %d", rc);

    return rc;
}

static struct ncr_data ncr_data;

DEVICE_DT_INST_DEFINE(0, &ncr_init, NULL, &ncr_data, NULL, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &ncr_api);
