Custom keyboard: Dactyl Manuform 5x6 (wireless)

Repository for ZMK firmware — a custom ZMK "shield" for a wireless split
Dactyl Manuform 5x6 keyboard. This is not a standalone app; it is consumed by
the ZMK build system as a Zephyr module and produces firmware .uf2 files via
GitHub Actions.


CONTROLLER
----------
Supermini nRF52840 — a nice!nano v2 drop-in replacement. All builds therefore
target the nice_nano_v2 board.

  Controller docs / pinout schematic:
  https://nicekeyboards.com/docs/nice-nano/pinout-schematic


BUILD
-----
There is no local build. Firmware is built in CI:

- Pushing, opening a PR, or manually triggering the workflow runs
  .github/workflows/build.yml (delegates to the shared ZMK
  build-user-config workflow, pinned to v0.3).
- The build matrix comes from build.yaml. It produces one .uf2 per half:
  shields dactyl-manuform-5x6_left and dactyl-manuform-5x6_right on
  nice_nano_v2.
- Built firmware is published as downloadable workflow artifacts.

To build a new combination, add a board + shield entry to build.yaml.

See docs/FLASHING.md for the full build -> download -> flash steps and for
where to change your keymap.


FEATURES
--------
- Wireless split over Bluetooth (BLE). The LEFT half is the split central
  (talks to the PC); the RIGHT half is a peripheral linked to the left.
- 5 independent BLE profiles for switching between devices without re-pairing.
- USB or BLE output, toggleable from the keymap (&out OUT_TOG).
- 12-column x 6-row col2row diode matrix; right half mirrored via col-offset.
- Keymap with 3 layers: default (Colemak), lower (symbols / numpad / nav),
  and raise (Bluetooth, output toggle, bootloader, function & media keys).
- One 18650 Li-ion cell per half (Panasonic NCR18650B, 3400 mAh).
- Battery-level LED on each half (see below).

Keymap file:
  boards/shields/dactyl-manuform-5x6/dactyl-manuform-5x6.keymap


BATTERY
-------
Each half is powered by its own cell: a Panasonic NCR18650B (Li-ion 18650,
MH12210, made in Japan).

- Nominal voltage: 3.6 V (3.7 V typical marking); charge to 4.20 V max,
  discharge cutoff 2.5 V.
- Capacity: 3400 mAh typical / 3250 mAh minimum.
- Size / weight: 18.5 x 65.3 mm, ~48.5 g. Flat-top, button-top adds ~2 mm.
- This is a bare (unprotected) cell — it has no protection PCB. Charging and
  cutoff are handled by the charger on the nRF52840 controller; do not
  short it, over-discharge it, or charge it outside the controller.
- Charging runs at ~300 mA (the BOOST pad is bridged), so a 3400 mAh cell needs
  roughly 11 hours from empty. Unplugging earlier leaves it part-charged — and
  the level shown while USB is connected does not reveal that (see below).

Wiring: cell + to the controller's battery/B+ pad, cell - to GND, on both
halves. Two cells are needed, one per half; they are never connected to each
other. See the nice!nano pinout schematic linked above for the battery pads.

Item 1 in docs/FUTURE-IMPROVEMENTS.md covers adding an in-line ON/OFF switch so
a stored keyboard does not drain its cells.


HOW THE BATTERY PERCENTAGE IS MEASURED
--------------------------------------
There is no fuel gauge chip. The level is estimated from the voltage on the
nRF52840's VDDH rail (SAADC VDDHDIV5), which is the cell when running on
battery. src/battery_ncr18650.c maps that voltage through an NCR18650B
discharge curve; both the Windows percentage and the LED blink count come from
that one number. Roughly:

  4.20 V ~ 100%   4.00 V ~ 80%   3.80 V ~ 55%   3.70 V ~ 40%
  3.60 V ~  26%   3.50 V ~ 15%   3.20 V ~  0%

0% is placed at 3.20 V, not the cell's 2.5 V cutoff — the cells are unprotected,
so the firmware calls empty while a safe margin is left.

Two things follow from measuring a voltage rather than counting charge:

- While USB is connected VDDH is the USB rail, not the cell, so the reading is
  meaningless. The firmware holds the last level measured on battery instead of
  jumping to 100%; a real reading resumes about a minute after unplugging.
- The level is an estimate. It moves fastest right after a charge and again at
  the very bottom, because that is where cell voltage moves fastest.

Stock ZMK converts the same voltage with a single straight line from 3450 mV
(0%) to 4200 mV (100%). That spends a third of the scale on the first tenth of
the capacity and squeezes the long 3.9-3.7 V plateau — where most of the
capacity lives — into ~25 points, which is why the level used to collapse to
~65% overnight, sit for a month, then fall off a cliff. See item 4 in
docs/FUTURE-IMPROVEMENTS.md.


BATTERY LED
-----------
A green LED on each half blinks its own half's battery level. It stays dark
otherwise, so idle power draw is ~zero.

- Blink count = level: 3 = >80%, 2 = 30-80%, 1 = <=30%.
- Boot: announces the level once on the first battery reading.
- Critical (<=15%, ~3.50 V): one blink every 30 s instead of a steady light.
- On demand: &batt_led_show on the raise layer blinks both halves (the right
  blinks only while the split link is connected).

Hardware: nexus D2 (P0.17), active-high, via 680R to GND, on both controllers.
Tunables (thresholds, blink timing) live in Kconfig; details and the soldering
schema are in docs/FUTURE-IMPROVEMENTS.md (item 2).


DOCS
----
- docs/FLASHING.md   - build, flash both halves, and edit the keymap
- docs/BLUETOOTH.md  - pair, switch, and troubleshoot Bluetooth
- docs/pin-mapping.md - QMK -> ZMK pin assignments
- ZMK documentation: https://zmk.dev/docs
