# Future Improvements — Dactyl Manuform 5x6 (ZMK)

_Last updated: 2026-09-02_

Hardware/firmware ideas. Item 1 is not done yet; item 2 (battery-level LED) and
item 4 (accurate battery percentage) are **implemented on both halves**; item 3
(RGB upgrade) is a future goal.

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

## 4. Accurate battery percentage — ✅ DONE

The reported level used to jump to 100% on every charge, fall to ~65% overnight,
sit there for a month, then collapse from 30-40% to under 10% in days. Neither
the cells nor the LED code were at fault — the level came straight from stock
ZMK, which:

- measures the **VDDH supply rail** (`zmk,battery-nrf-vddh`, SAADC `VDDHDIV5`
  ×5). While USB is connected that rail is fed from USB through the input diode
  and reads well above 4.2 V, so the answer is always 100% no matter what the
  cell holds;
- converts mV to percent with one straight line,
  `pct = mv * 2 / 15 - 459`, clamped to 0% at 3450 mV and 100% at 4200 mV.

That line is the whole story: 65% is exactly 3930 mV, 40% is 3742 mV, 10% is
3517 mV. An NCR18650B drops 4.20 → 3.93 V over roughly the first 10-15% of its
capacity (35 points of the scale), then holds 3.9-3.7 V for most of its capacity
(only ~25 points), then falls away below 3.65 V.

### What was changed

`src/battery_ncr18650.c` — a drop-in replacement sensor, compatible
`dactyl,battery-ncr18650`, selected by `chosen { zmk,battery = &ncr_batt; }` in
`dactyl-manuform-5x6.dtsi` (which also disables `&vbatt`). It samples VDDH
identically to ZMK's driver and then:

- maps voltage through an NCR18650B discharge curve
  (`4200→100, 4000→80, 3800→55, 3700→40, 3600→26, 3500→15, 3200→0`, linearly
  interpolated). **0% sits at 3.20 V**, not the cell's 2.5 V cutoff, because
  these are bare cells with no protection PCB;
- **holds the last battery-powered reading while USB is powered**
  (`CONFIG_DACTYL_BATTERY_HOLD_WHILE_CHARGING`, via `zmk_usb_is_powered()`)
  instead of reporting the rail. A real reading resumes on the first sample
  after unplugging, within `CONFIG_ZMK_BATTERY_REPORT_INTERVAL` (60 s).

Supporting changes: `CONFIG_ZMK_BATTERY_REPORTING_FETCH_MODE_STATE_OF_CHARGE=y`
in the shield `.conf` so ZMK takes the driver's percentage rather than
re-deriving one from voltage; LED thresholds retuned for the new scale
(3 blinks >80%, 2 blinks >30%, critical heartbeat ≤15% ≈ 3.50 V); and
`ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING`/`_PROXY` on the central so the
right half's level also reaches the host.

Every sample logs `vddh <n> mV -> <n>%` at debug level — capture that across a
real discharge to recalibrate the curve points against this keyboard's own load.

### Still worth checking on the hardware side

The curve fixes the *reporting*. It does not explain a cell resting at 3.93 V
the morning after a charge: a cell that finishes a CV charge rests at
4.15-4.18 V, and the month-long plateau proves idle draw is only ~1-5 mA, far
too little to pull 200 mV off the top of the curve overnight. So the cell is
probably not reaching full charge. Charging runs at 300 mA (BOOST bridged) —
~11 h from empty for 3400 mAh, ~4 h from 65%. Candidates, in order:

1. **USB unplugged too early.** Charge until the board's charge LED reports
   done, unplug, rest 30 min, measure at the terminals: ≥4.15 V is healthy.
2. **Charge never saturates because the keyboard is running** — its load sits in
   parallel with the cell on the charger's BAT pin and can defeat the
   current-based termination. Retest with the halves off (item 1's switch), or
   charge the cells in an external 18650 charger.
3. **Counterfeit cells.** Green NCR18650B wraps are among the most faked; an
   external charger that reports delivered mAh settles it — a genuine cell takes
   ≥3000 mAh from empty, fakes are often 1200-2000 mAh.
