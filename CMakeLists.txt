# Compile into ZMK's `app` target so these sources inherit ZMK's private
# include path (zmk/..., drivers/behavior.h). A standalone zephyr_library()
# would not see those headers.
#
# The behavior is compiled unconditionally so the `&batt_led_show` keymap
# reference resolves on both halves (it is a weak no-op on a half with no LED).
# The LED driver itself is only built where a gpio-leds node exists.
target_sources(app PRIVATE src/behavior_batt_led.c)
target_sources_ifdef(CONFIG_DACTYL_BATT_LED app PRIVATE src/battery_led.c)
target_sources_ifdef(CONFIG_DACTYL_BATTERY_CURVE app PRIVATE src/battery_ncr18650.c)
target_sources_ifdef(CONFIG_DACTYL_BATTERY_CUTOFF app PRIVATE src/battery_cutoff.c)
