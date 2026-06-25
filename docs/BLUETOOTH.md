# Bluetooth Management — Dactyl Manuform 5x6 (ZMK)

_Last updated: 2026-06-25_

How to pair, switch, and troubleshoot Bluetooth on the wireless Dactyl Manuform 5x6.

## Key concept: the left half owns Bluetooth

The **left half is the split central**. It is the only half that talks to your
computer over Bluetooth — the right half is a peripheral that connects to the left,
not to the PC. So **every Bluetooth action happens on the left half**, and both halves
must be powered and linked for the BT keys to work (the layer key is on the right, the
BT keys are on the left).

ZMK keeps **5 independent BLE profiles** (slots 0–4). Each profile remembers one bonded
device. Switching profiles is how you move the keyboard between, e.g., a desktop and a
laptop without re-pairing.

## The Bluetooth keys (Raise layer)

All BT keys live on the **Raise layer**. Reach it by holding the **Raise** key
(`&lt RSE PSCRN`, right pinky), then press:

- `&bt BT_SEL 0` … `&bt BT_SEL 3` — select / switch to BLE profile 0–3.
- `&bt BT_CLR` — clear the bond on the **currently selected** profile (forget that one
  device on the keyboard side) and re-open it for pairing.
- `&out OUT_TOG` — toggle keyboard output between **BLE** and **USB**. If keys work over
  USB but not Bluetooth, check this first — you may have toggled to USB output.
- `&bootloader` — reboots the left half into the UF2 bootloader (for flashing, not BT).

> Note: `&bt BT_CLR` only clears the **selected** profile. There is no `BT_CLR_ALL` bound
> in this keymap — to wipe every profile at once, flash the settings-reset firmware
> (see "Full reset" below).

## Pair a new device

1. Hold **Raise**, press `&bt BT_SEL n` for an **empty** profile slot.
2. On the computer: Bluetooth settings → **Add device → Bluetooth** → select **"Dactyl 5x6"**.
3. It bonds and connects. That profile now remembers this device.

## Switch between paired devices

Hold **Raise** and press `&bt BT_SEL n` for the profile of the device you want. The
keyboard reconnects to whatever is bonded on that slot. No re-pairing needed.

## Fix "can't connect / try again" bond errors

**Symptom:** you removed the keyboard in the OS Bluetooth menu, the keyboard still shows
up in the list, but connecting fails with an error / "try again."

**Cause:** a **one-sided bond removal**. The OS forgot the keyboard, but the keyboard
still holds the old pairing keys for that profile. The encryption handshake no longer
matches, so the connection is rejected.

**Fix (this is the procedure that worked on 2026-06-25):**

1. Hold **Raise**, press `&bt BT_SEL n` for the profile the OS was using (default is 0).
2. Still holding Raise, press **`&bt BT_CLR`** — this erases the keyboard-side bond for
   that profile and re-opens it for pairing.
3. In the OS, **Remove device** again if the keyboard is still listed.
4. **Add device → Bluetooth → "Dactyl 5x6"** → it pairs cleanly.

**If it still fails:**
- Pair on a **fresh, never-used profile slot** instead — no stale bond to clash with.
- Toggle the OS Bluetooth adapter off/on (or reboot) so it drops its cached record.
- Confirm output is **BLE**, not USB (`&out OUT_TOG`).

## Full reset (wipe ALL profiles)

When per-profile `&bt BT_CLR` isn't enough (e.g. persistent bond corruption across all
slots), flash the **settings-reset** firmware to the **left half** (the central holds all
bonds):

1. Build/download the `settings_reset` UF2 for `nice_nano_v2` (ZMK docs → "Reset split
   keyboard", or add a `settings_reset` shield to `build.yaml`).
2. Enter the bootloader on the **left** half (double-tap reset, or `&bootloader`) and copy
   the settings-reset UF2 onto it.
3. Re-flash the normal `dactyl-manuform-5x6_left` firmware afterward.
4. For a stubborn split-link issue, do the same settings-reset on the **right** half too,
   then re-flash normal firmware to both and power them on together so they re-discover.

This erases every stored pairing on the keyboard — you'll re-pair all devices from scratch.

## Split re-pairing note

If the two halves stop talking to each other (right not registering after a reset/flash),
settings-reset **both** halves, flash normal firmware to both, then power them on together.
The right (peripheral) re-discovers the left (central) automatically within a few seconds.

## Reference

- ZMK Bluetooth behaviors: https://zmk.dev/docs/keymaps/behaviors/bluetooth
- ZMK split / settings reset: https://zmk.dev/docs/troubleshooting/connection-issues
