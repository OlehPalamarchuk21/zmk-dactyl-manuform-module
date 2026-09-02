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

Wiring: cell + to the controller's battery/B+ pad, cell - to GND, on both
halves. Two cells are needed, one per half; they are never connected to each
other. See the nice!nano pinout schematic linked above for the battery pads.

Note the charge current of the Supermini nRF52840 is small compared to the
cell's capacity, so a full charge from empty takes many hours. Item 1 in
docs/FUTURE-IMPROVEMENTS.md covers adding an in-line ON/OFF switch so a stored
keyboard does not drain its cells.


BATTERY LED
-----------
A green LED on each half blinks its own half's battery level. It stays dark
otherwise, so idle power draw is ~zero.

- Blink count = level: 3 = >90%, 2 = 20-90%, 1 = <=20%.
- Boot: announces the level once on the first battery reading.
- Critical (<=10%): one blink every 30 s instead of a steady light.
- On demand: &batt_led_show on the raise layer blinks both halves (the right
  blinks only while the split link is connected).

Hardware: nexus D2 (P0.17), active-high, via 680R to GND, on both controllers.
Tunables (thresholds, blink timing) live in Kconfig; details and the soldering
schema are in docs/FUTURE-IMPROVEMENTS.md (section 3).


DOCS
----
- docs/FLASHING.md   - build, flash both halves, and edit the keymap
- docs/BLUETOOTH.md  - pair, switch, and troubleshoot Bluetooth
- docs/pin-mapping.md - QMK -> ZMK pin assignments
- ZMK documentation: https://zmk.dev/docs
