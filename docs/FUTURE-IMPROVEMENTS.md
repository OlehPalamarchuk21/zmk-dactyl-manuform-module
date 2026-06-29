# Future Improvements — Dactyl Manuform 5x6 (ZMK)

_Last updated: 2026-06-29_

Hardware/firmware ideas. Item 1 is not done yet; item 2 (battery-level LED) is
**implemented on both halves**; item 3 (RGB upgrade) is a future goal.

## 1. Add a physical battery switch

Add a hardware ON/OFF switch in line with each battery, one per half.

- **Why:** fully cuts battery drain when the keyboard is stored or shipped — no
  reliance on deep-sleep. A clean power cycle also helps recover a half that has
  hung or lost its split link.
- **Notes:** wire the switch on the battery positive lead to the controller's
  battery input. Use a small slide/toggle switch that fits the case. Do this on
  **both** halves, since each carries its own battery.

## 2. Battery-level LED — ✅ DONE

A green discrete LED on each half blinks that half's own battery level; it stays
dark otherwise, so idle draw is ~zero.

Behavior (each LED reflects its own half's battery):

- **Blink count = level:** 3 blinks `>90%`, 2 blinks `>20%`, 1 blink `≤20%`.
- **Critical heartbeat:** at `≤10%`, one blink every 30 s instead of a steady
  light, to save power.
- **Boot:** announces the level once on the first battery reading.
- **On demand:** the `&batt_led_show` keymap behavior (raise layer) blinks both
  halves — each its own level; the right blinks only while the split link is
  connected.

Where it lives: `src/battery_led.c` (state machine + battery listener),
`src/behavior_batt_led.c` (the `&batt_led_show` global behavior),
`dts/bindings/behaviors/zmk,behavior-batt-led.yaml`, `Kconfig`
(`CONFIG_DACTYL_BATT_LED` + thresholds), and the `batt-led` `gpio-leds` node in
both shield overlays.

### Hardware — pin & LED connection

- **Pin: nexus `2` (D2 = P0.17), active-high**, on **both** controllers. D2 sits
  immediately below a GND pad on the board edge — signal and ground on adjacent
  pads, the easiest solder job. (Other free pins: `0,1,3,20,21`; the matrix uses
  cols `4–9`, rows `19,18,15,14,16,10`.)
- **Resistor: 680 Ω, ¼ W.** GPIO high ≈ 3.3 V, green Vf ≈ 2.2 V →
  `(3.3 − 2.2) / 680 ≈ 1.6 mA` (frugal, clearly visible). Use 470 Ω → ≈ 2.3 mA
  for a brighter blink. Always include the resistor.
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

Devicetree node (present in each shield overlay):

```dts
/ {
    aliases { batt-led = &batt_led; };
    leds {
        compatible = "gpio-leds";
        batt_led: batt_led {
            gpios = <&pro_micro 2 GPIO_ACTIVE_HIGH>;
        };
    };
};
```

## 3. Upgrade the battery LED to an RGB LED

Replace the single green LED with a **discrete analog RGB LED** (common-anode or
common-cathode) so level can be shown by **colour** (green/yellow/red) as well as
— or instead of — blink count.

- **Why:** colour is read at a glance, no blink-counting. A *discrete* (not
  addressable) RGB LED keeps the zero-idle-draw property — it draws current only
  while lit, unlike WS2812/SK6812 pixels whose controller IC sips ~0.5–1 mA 24/7
  and would noticeably cut battery life.
- **Hardware:** 3 free GPIOs + 3 current-limiting resistors per half (one per
  R/G/B leg), plus the common pin to GND (common-cathode) or 3V3 (common-anode).
  Free nexus pins are `0,1,3,20,21` (D2/P0.17 already drives the current green
  LED). Size each resistor for its colour's forward voltage (red ≈ 1.8 V,
  green/blue ≈ 2.0–3.2 V) off the 3.3 V GPIO.
- **Firmware:** declare a 3-channel `gpio-leds` node (or `pwm-leds` for blended
  colours) and extend `src/battery_led.c` to pick colour by level. Alternatively
  pull in `caksoylar/zmk-rgbled-widget` via `config/west.yml` — it already does
  green/yellow/red-by-level over a plain `gpio-leds` node.
- **Notes:** keep blink-on-change rather than steady-on to save power; add one
  per half; avoid addressable strips unless a power-gate FET is added.
