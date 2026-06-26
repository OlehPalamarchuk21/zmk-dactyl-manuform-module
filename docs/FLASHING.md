# Flashing Firmware & Changing Keymaps — Dactyl Manuform 5x6 (ZMK)

_Last updated: 2026-06-26_

How to build the firmware, flash both halves of the keyboard, and where to edit
your keymap.

## Key concept: there is no local build

This repo is a ZMK **shield module**, not an app you compile on your machine.
Firmware is built **in GitHub Actions** every time you push, and you download the
resulting `.uf2` files as workflow artifacts. You flash each half by copying its
`.uf2` onto the controller while it sits in the UF2 bootloader.

The board is a Supermini nRF52840 (a `nice_nano_v2` drop-in), so all builds target
`nice_nano_v2`. The two shields are `dactyl-manuform-5x6_left` and
`dactyl-manuform-5x6_right` — you get one `.uf2` per half.

---

## Part 1 — Build the firmware (CI)

1. **Commit and push** your changes (or just push anything) to GitHub. A push, a PR,
   or a manual run all trigger `.github/workflows/build.yml`.
   - To trigger manually: GitHub repo → **Actions** tab → **Build ZMK firmware** →
     **Run workflow**.
2. Open the **Actions** tab and click the most recent **Build ZMK firmware** run.
3. Wait for the run to finish (green check). It builds both halves from the matrix in
   `build.yaml`.
4. Scroll to the **Artifacts** section at the bottom of the run summary and download
   the firmware zip. Inside you'll find two files:
   - `dactyl-manuform-5x6_left-nice_nano_v2-zmk.uf2`
   - `dactyl-manuform-5x6_right-nice_nano_v2-zmk.uf2`
5. Unzip it. These two `.uf2` files are what you flash in Part 2.

> If the build fails, open the failed job's log in the Actions tab — keymap typos
> (unknown behavior/keycode) show up there as compile errors.

---

## Part 2 — Flash each half

You flash **both halves the same way**, using each half's matching `.uf2`. Do them
one at a time.

1. **Connect** the half to the computer with a USB cable (use a real data cable, not
   charge-only).
2. **Enter the UF2 bootloader** on that half, either way:
   - **Double-tap the reset button** on the controller quickly, **or**
   - Use the keymap key: hold **Raise** (right pinky) and press `&bootloader`
     (top-left key on the Raise layer). *This only reboots the **left** half into the
     bootloader — for the right half use the physical reset button.*
3. A USB drive named **`NICENANO`** appears in your file manager.
4. **Copy the matching `.uf2`** onto that drive:
   - Left half → `dactyl-manuform-5x6_left-...uf2`
   - Right half → `dactyl-manuform-5x6_right-...uf2`
5. The drive auto-ejects and the half reboots into the new firmware. Done.
6. Repeat for the other half.

After both halves are flashed, power them on together so the right (peripheral)
re-discovers the left (central). For Bluetooth pairing afterward, see
[BLUETOOTH.md](BLUETOOTH.md).

> **Which half is which:** the **left** half is the split central — it's the one that
> talks to your computer over Bluetooth and carries the keyboard name. Always flash
> the left `.uf2` to the left half and the right `.uf2` to the right half; they are
> not interchangeable.

---

## Part 3 — Where to change your keymap

Your keymap lives in **one file**:

```
boards/shields/dactyl-manuform-5x6/dactyl-manuform-5x6.keymap
```

It defines **3 layers**, selected by the `#define`s at the top:

- `default_layer` (DEF, layer 0) — the base layer. Currently a **Colemak** layout.
- `lower_layer` (LWR, layer 1) — symbols, numpad, arrows, media nav. Reached by holding
  the **Lower** thumb key (`&mo LWR`).
- `raise_layer` (RSE, layer 2) — Bluetooth, output toggle, bootloader, function keys,
  media controls. Reached by holding the **Raise** key (`&lt RSE PSCRN`, right pinky).

### How to edit a key

Each layer is a grid of **bindings** in physical key order (left half first, then right
half, row by row, with the thumb cluster rows at the bottom). To change a key, replace
its binding. Common forms:

- `&kp KEY` — tap a keycode, e.g. `&kp A`, `&kp ESC`, `&kp F5`.
- `&mt MOD KEY` — **mod-tap**: hold = modifier, tap = key, e.g. `&mt LSHFT SPACE`.
- `&lt LAYER KEY` — **layer-tap**: hold = switch to layer, tap = key, e.g. `&lt RSE PSCRN`.
- `&mo LAYER` — momentary layer (active only while held), e.g. `&mo LWR`.
- `&kp LC(Z)` / `&kp LG(SPACE)` — modified keys (Ctrl+Z, Cmd/Win+Space).
- `&bt`, `&out`, `&bootloader` — Bluetooth / output / bootloader behaviors.
- `&none` — does nothing. `&trans` — transparent (falls through to the layer below).

Keep the grid alignment readable, but only the **order** of bindings matters, not the
whitespace. Keycode names come from ZMK's
[keymap reference](https://zmk.dev/docs/keymaps/list-of-keycodes).

### After editing

1. Save the file and **push** (or open a PR). That re-runs the build (Part 1).
2. Download the new artifacts and **flash both halves** (Part 2).
3. There is no way to test the keymap without flashing — the build is CI-only.

> Tip: keymap changes that don't touch GPIO/matrix usually affect both halves' firmware,
> so re-flash **both** halves after any keymap edit, even if you only changed one side's
> keys.

---

## Quick reference

- Keymap file: `boards/shields/dactyl-manuform-5x6/dactyl-manuform-5x6.keymap`
- Build matrix: `build.yaml` (add `board`/`shield` combos here)
- Bootloader drive name: `NICENANO`
- Targets: `nice_nano_v2`, shields `dactyl-manuform-5x6_left` / `_right`
- ZMK keymaps & behaviors: https://zmk.dev/docs/keymaps
- ZMK keycode list: https://zmk.dev/docs/keymaps/list-of-keycodes
