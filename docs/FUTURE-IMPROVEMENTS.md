# Future Improvements — Dactyl Manuform 5x6 (ZMK)

_Last updated: 2026-06-28_

Hardware/firmware ideas. Items 1–2 are not done yet; item 3 (battery-level LED)
is **implemented on the right half only** (it reflects the right half's own
battery).

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

## 3. Add a LED for battery-level indication — ✅ DONE

**Implemented** on the **right half only**: a green discrete LED on nexus `D2`
(active-high, via 680 Ω to GND), driven by a small in-repo ZMK module. The left
half has no LED node, so the LED driver is not compiled into the left build.

Behavior (the LED reflects the right half's own battery):

- **Blink count = level:** 3 blinks `>70%`, 2 blinks `>30%`, 1 blink `≤30%`.
- **Critical heartbeat:** at `≤10%`, one blink every 30 s (repeating) instead of
  a steady light, to save power.
- **Boot:** announces the level once on the first battery reading.
- **On demand:** the `&batt_led_show` keymap behavior, on the **raise** layer
  (right half, bottom main row, innermost key). It has `BEHAVIOR_LOCALITY_GLOBAL`
  so the keypress fires on both halves, but it's a no-op on the left (no LED);
  only the right LED blinks, and only while the split link is connected so the
  press reaches the right half.

Where it lives:

- `src/battery_led.c` — LED state machine + battery listener (`batt_led_show()`).
- `src/behavior_batt_led.c` — the `&batt_led_show` behavior (global locality).
- `dts/bindings/behaviors/zmk,behavior-batt-led.yaml` — behavior binding.
- `Kconfig` — `CONFIG_DACTYL_BATT_LED` + thresholds (`..._LEVEL_HIGH=70`,
  `..._LEVEL_LOW=30`, `..._LEVEL_CRITICAL=10`, `..._CRITICAL_INTERVAL_SEC=30`,
  `..._BLINK_MS=200`); tunable from the shield `.conf`.
- `CMakeLists.txt` — adds the sources to ZMK's `app` target (so `zmk/...`
  headers resolve); LED driver built only where a `gpio-leds` node exists.
- Only the right overlay declares the `batt-led` gpio-leds node; `.conf` sets
  `CONFIG_ZMK_BATTERY_REPORTING=y`; `zephyr/module.yml` adds `cmake`/`kconfig`/
  `dts_root`.

The original design notes below are kept for rationale and the soldering schema.

### Notes / troubleshooting — right (peripheral) LED

The right LED stayed dark while the left worked, with identical wiring and code.
Cause: the right half was still running **old firmware** built before the
`batt-led` node was added to `dactyl-manuform-5x6_right.overlay`, so that build
had no LED code at all. **Fix: flash the latest `dactyl-manuform-5x6_right…uf2`
to the right half specifically** (not the `_left` file). After reflashing both
halves with the current build, both LEDs work, including the hotkey.

Things to remember when the right LED misbehaves:

- **Reflash the matching half.** Each half needs its own `_left` / `_right`
  artifact; flashing `_left` to both leaves the right with no LED firmware.
- **Right responds to the hotkey only while connected.** The key press is
  processed on the central (left) and forwarded to the peripheral (right) via
  the split link (`BEHAVIOR_LOCALITY_GLOBAL`). If the halves aren't paired/
  connected at that moment, the right LED won't blink on the keypress — though
  its own boot-announce and critical heartbeat still run independently.
- **If still dark after reflashing the latest `_right`:** suspect hardware on
  the right LED only (left is proven good) — check LED polarity (long leg/anode
  toward the resistor & D2, short leg/cathode to GND) and the solder joints on
  D2 and the adjacent GND pad for continuity.

### Original idea

Solder a small LED to a free GPIO on the controller and drive it from ZMK to
signal battery state (e.g. blink a few times on boot, or warn when low).

- **Why:** quick visual check of remaining charge without opening the host's
  Bluetooth/battery UI. Useful for catching a low battery before the half dies
  mid-use.
- **How (hardware):** pick a free `&pro_micro` pin (the matrix already uses
  columns `4–9` and rows `19, 18, 15, 14, 16, 10`, so avoid those — typical free
  pins are `0, 1, 2, 3, 20, 21`). Wire `GPIO → current-limiting resistor
  (~330Ω–1kΩ) → LED → GND` (active-high) or `3V3 → resistor → LED → GPIO`
  (active-low). Always include the resistor; nRF52840 GPIOs source only a few mA.
- **How (firmware):** declare a `gpio-leds` node in the shield overlay, then
  either pull in a single-LED status module via `config/west.yml`
  (e.g. zmk-poor-mans-led-indicator or zmk-batt-led-indicator) and set its
  `CONFIG_*_BATTERY_LEVEL_*` thresholds in the shield `.conf`, or write a custom
  behavior that toggles the LED on the battery state-changed event. For an RGB
  LED, caksoylar/zmk-rgbled-widget blinks green/yellow/red by level.
- **Notes:** prefer blink-on-boot / blink-on-change over a constantly-lit LED to
  avoid draining a wireless battery. The LED only reflects the half it is soldered
  to, so add one per half if both halves should be indicated. Confirm the chosen
  pad is a genuinely free, broken-out GPIO (not a power/analog/SWD pin) on the
  controller schematic before soldering:
  https://nicekeyboards.com/docs/nice-nano/pinout-schematic

### Choosing the LED type — power efficiency (for future investigation)

The key realization: the ~0.5–1 mA idle drain is a property of *addressable*
LEDs (WS2812 / SK6812), not of LEDs in general. Every WS2812/SK6812 contains a
small controller IC that is powered 24/7 and sips ~0.5–1 mA whenever its VDD is
live, regardless of what color (even "off") you send — there is no power-off
command in the protocol. A plain "dumb" / discrete LED has **no idle draw at
all**: when its GPIO is low, essentially zero current flows (nanoamp leakage),
and it only uses power during the brief moments it actually blinks.

Why that matters here: a well-configured ZMK split on nRF52840 averages only
~0.05–0.3 mA when connected and idle (hence weeks-to-months of life on a
~100–500 mAh LiPo). So a continuous 0.5–1 mA from an addressable LED is in the
same league as — or larger than — the entire keyboard, and can roughly halve
battery life or worse. Example on a 300 mAh cell: the LED idle alone (~0.75 mA)
would flatten it in ~16 days; on a 110 mAh cell, ~6 days. The blink energy
itself is negligible — the idle current was the whole problem.

Options, roughly best-to-worst on power:

1. **Single discrete LED (one color)** — most efficient, simplest. 1 GPIO + 1
   resistor, **zero idle draw**, only ~1–2 mA while blinking. Encode battery
   level by blink *pattern* (e.g. 3 blinks = good, 2 = medium, 1 = low) instead
   of color. Uses only one pin.
2. **Discrete analog RGB LED (common-anode/cathode)** — efficient *and* color
   coded. 3 GPIOs + 3 resistors, **zero idle draw**, lights only when blinking.
   Gives green/yellow/red-by-level. This is the mode `caksoylar/zmk-rgbled-widget`
   was originally built for, via a plain `gpio-leds` node (no `led-strip` driver,
   no power-gating FET needed). Costs 3 of the ~6 free pins (`0,1,2,3,20,21`).
3. **Two discrete LEDs (e.g. green + red)** — middle ground. 2 GPIOs + 2
   resistors, zero idle. Green = good, red = low, both = amber-ish.
4. **WS2812B strip / SK6812 MINI-E** — least efficient at idle. The SK6812
   MINI-E is smaller than a 5050 strip but has the same ~0.5–1 mA parasitic
   idle, so it is not more power-effective. The only way to make any addressable
   LED frugal is a hardware power-gate.

A software key-combo on/off does **not** remove the addressable-LED idle draw —
it only changes the color, while the IC stays powered. To truly cut it you must
remove VDD: either a physical slide switch in the LED's power lead, or a
P-channel MOSFET / load-switch between the 3V3 rail and the LED's `+5V` pad with
its gate driven by a free GPIO (key combo → GPIO → FET → LED power). Costs one
extra GPIO plus the FET. Do not power a 5050 pixel's VDD directly from a GPIO —
it can pull far more than an nRF52840 pin can safely source.

If using the existing WS2812B addressable strip: cut a single segment at a
cut-line, wire `GPIO → DIN` (not `DO` — follow the data-direction arrow),
`3V3 → +5V`, `GND → GND`. Power it from **3V3, not 5V**: the GPIO swings only
0–3.3V and WS2812B logic-high is ~0.7×VDD, so a 5V supply makes the 3.3V data
signal marginal; a 3V3 supply drops the threshold to ~2.3V which the GPIO drives
cleanly (slightly dimmer, but reliable).

**Recommendation:** for best battery life with color coding, use a **discrete
analog RGB LED on 3 GPIOs** (option 2) — zero idle, blink-only, green/yellow/red,
no extra FET, directly supported by the widget. For the absolute lowest power /
fewest pins, a **single discrete LED with blink-count patterns** (option 1).
Reserve the WS2812B/SK6812 path only if a power-gate FET is added.

### Chosen plan: single green discrete LED (option 1)

Concrete decisions for when this gets built. Battery level is encoded by **blink
count** (e.g. 3 blinks = good, 2 = medium, 1 = low), so color is a free choice;
green is picked for the best visibility-per-milliamp (near the eye's peak
sensitivity ~555 nm) while keeping comfortable resistor headroom on the 3.3 V
GPIO. (Blue/white are avoided — their ~3.0–3.4 V forward voltage leaves too little
headroom to current-limit reliably from a 3.3 V pin.)

- **Pin: nexus `2` (D2 = P0.17), active-high.** On the nice!nano / Supermini
  left column the order top-to-bottom is `TX(1), RX(0), GND, GND, D2, D3, D4…`,
  so **D2 sits immediately below a GND pad** — signal and ground land on adjacent
  board-edge pads, the easiest possible solder job. Free pins are `0,1,2,3,20,21`
  (matrix uses cols `4–9`, rows `19,18,15,14,16,10`); `20/21` are also free but
  their nearest GND is several pads away, so D2 wins for wiring comfort.
- **Resistor: 680 Ω, ¼ W.** With GPIO high ≈ 3.3 V and green Vf ≈ 2.2 V,
  `R = (3.3 − 2.2) / I`. 680 Ω → ≈ 1.6 mA (frugal, clearly visible for a blink);
  use 470 Ω → ≈ 2.3 mA if a brighter blink is wanted. Always include the resistor.
- **Polarity:** long leg = anode → toward resistor / D2; short leg (flat edge of
  the rim) = cathode → GND. Reversed = no light.

Soldering schema (active-high: D2 HIGH = lit):

```
   Supermini / nice!nano v2 — left edge (USB at top)
   ┌──────────────┐
   │  TX  (1) ○   │
   │  RX  (0) ○   │
   │  GND     ●───────────────────────┐     ← GND pad (the one just above D2)
   │  GND     ○   │                    │
   │  D2  (2) ●──[ 680Ω ]──►|──────────┘
   │  D3  (3) ○   │          ▲  ▲
   │  D4  (4) ○   │          │  └─ cathode (short leg / flat side) → GND
   │  ...         │          └──── anode   (long leg) ← from resistor
   └──────────────┘                green LED

   Current path:  D2 ──► 680Ω ──► anode(+) ─ green LED ─ cathode(−) ──► GND
```

Firmware side — declare the LED in the left overlay, then drive it from a
single-LED battery-indicator module or a custom battery state-changed behavior:

```dts
/ {
    leds {
        compatible = "gpio-leds";
        batt_led: batt_led {
            gpios = <&pro_micro 2 GPIO_ACTIVE_HIGH>;
        };
    };
};
```

The LED reflects only the half it is soldered to (the left/central half here), so
add an identical one to the right half if both should be indicated. Mounting an
LED inside a brown MX switch is possible (MX housings have a north-face LED
window), but brown housings are usually opaque so only a focused point shows
through; a clear-top switch or a case-wall mount near the controller reads better.
