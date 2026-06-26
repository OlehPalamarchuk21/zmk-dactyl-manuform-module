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

Keymap file:
  boards/shields/dactyl-manuform-5x6/dactyl-manuform-5x6.keymap


DOCS
----
- docs/FLASHING.md   - build, flash both halves, and edit the keymap
- docs/BLUETOOTH.md  - pair, switch, and troubleshoot Bluetooth
- docs/pin-mapping.md - QMK -> ZMK pin assignments
- ZMK documentation: https://zmk.dev/docs
