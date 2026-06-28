# The behavior is compiled unconditionally so the `&batt_led_show` keymap
# reference resolves on both halves (it is a weak no-op on the half without the
# LED). The LED driver itself is only built where a gpio-leds node exists.
zephyr_library()
zephyr_library_sources(src/behavior_batt_led.c)

if(CONFIG_DACTYL_BATT_LED)
  zephyr_library_sources(src/battery_led.c)
endif()
