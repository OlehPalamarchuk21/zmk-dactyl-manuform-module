# Future Improvements — Dactyl Manuform 5x6 (ZMK)

_Last updated: 2026-06-26_

Hardware/firmware ideas to tackle later. Not done yet.

## 1. Add a physical battery switch

Add a hardware ON/OFF switch in line with each battery, one per half.

- **Why:** fully cuts battery drain when the keyboard is stored or shipped — no
  reliance on deep-sleep. A clean power cycle also helps recover a half that has
  hung or lost its split link.
- **Notes:** wire the switch on the battery positive lead to the controller's
  battery input. Use a small slide/toggle switch that fits the case. Do this on
  **both** halves, since each carries its own battery.

## 2. Change the charging current via the solder jumper

The nice!nano v2 (and the Supermini nRF52840 drop-in) sets its LiPo charge
current with an on-board solder jumper / bridge.

- **Why:** match the charge current to the actual battery capacity. Small cells
  charge safer/cooler at a lower current; larger cells can take a higher one.
- **How:** bridge (or open) the charge-current solder pads on the controller to
  select the desired current. Confirm the exact pads and resulting current from
  the controller pinout/schematic before soldering:
  https://nicekeyboards.com/docs/nice-nano/pinout-schematic
- **Notes:** never set a current above your battery's rated charge limit. Apply
  the same change on **both** halves if both use the same battery.
