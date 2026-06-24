# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A [ZMK firmware](https://zmk.dev/docs) module that defines a custom **shield** for a wireless split **Dactyl Manuform 5x6** keyboard. It is not a standalone application — it is consumed by the ZMK build system as a Zephyr module and produces firmware `.uf2` files via GitHub Actions, not via a local build.

## Build

There is no local build command. Firmware is built in CI:

- Pushing, opening a PR, or triggering the workflow manually runs `.github/workflows/build.yml`, which delegates to the shared `zmkfirmware/zmk/.github/workflows/build-user-config.yml@v0.3` reusable workflow.
- The build matrix is generated from `build.yaml` (top-level `board`/`shield` arrays plus an `include` list for per-combination control such as `cmake-args`, `snippet`, and `artifact-name`).
- Built firmware is published as downloadable workflow artifacts (one `.uf2` per board+shield combination, typically a left and right half for this split board).

To build a new combination, add a `board` + `shield` entry to `build.yaml`. The shield names are `dactyl-manuform-5x6_left` and `dactyl-manuform-5x6_right`.

ZMK pins to `v0.3` in two places that must stay in sync: `config/west.yml` (`revision: v0.3`) and the workflow ref in `.github/workflows/build.yml`.

## Architecture

The repo is a Zephyr/ZMK module wired together by three layers:

- **`zephyr/module.yml`** — declares this repo as a Zephyr module and sets `board_root: .`, which tells ZMK to discover shields under `boards/` in this repo.
- **`config/west.yml`** — west manifest that imports upstream ZMK (`app/west.yml`) and registers this repo's `config/` as the manifest `self` path. This is how CI pulls ZMK and overlays this module.
- **`boards/shields/dactyl-manuform-5x6/`** — the shield definition itself.

### Shield definition (`boards/shields/dactyl-manuform-5x6/`)

Split keyboard shields in ZMK are two shields (`_left` and `_right`) sharing a base name:

- **`Kconfig.shield`** — declares the `SHIELD_DACTYL-MANUFORM-5X6_LEFT` / `_RIGHT` symbols, each set true when the matching name appears in the build's shield list.
- **`Kconfig.defconfig`** — sets defaults per half. The **left** half is the split central (`ZMK_SPLIT_ROLE_CENTRAL=y`) and carries the keyboard name (`ZMK_KEYBOARD_NAME="dactyl-manuform-5x6"`, which must stay under 16 characters); both halves enable `ZMK_SPLIT=y`.

### Shield files

- **`dactyl-manuform-5x6.dtsi`** — shared matrix transform (`columns = <12>`, `rows = <6>`) plus the `kscan0` col2row GPIO matrix. The `row-gpios` live here (shared by both halves); `col-gpios` are filled in per-half by the overlays.
- **`dactyl-manuform-5x6_left.overlay`** — left `col-gpios` (`&pro_micro 4..9`).
- **`dactyl-manuform-5x6_right.overlay`** — right `col-gpios` reversed (`&pro_micro 9..4`) with `col-offset = <6>` on the transform, which mirrors the right half into transform columns 6–11.
- **`dactyl-manuform-5x6.keymap`** — 4 layers: `qwerty`, `lower`, `raise`, and `adjust` (BLE profile select, USB/BLE output toggle, bootloader/reset). `adjust` is a tri-layer reached by holding both thumb layer keys (conditional-layers `lower`+`raise`).
- **`dactyl-manuform-5x6.zmk.yml`** — shield metadata declaring the left/right siblings and the `pro_micro` requirement.

### Hardware provenance

Pin assignments were ported from the original QMK build (`tshort` Dactyl Manuform 5x6, ATmega32u4 Pro Micro). The QMK AVR pin names map to ZMK's `&pro_micro` nexus positions as: columns `D4,C6,D7,E6,B4,B5` → `4,5,6,7,8,9`; rows `F6,F7,B1,B3,B2,B6` → `19,18,15,14,16,10`. The controller is a Supermini nRF52840 (a nice!nano v2 drop-in), so builds target `nice_nano_v2`. Diodes are COL2ROW.
