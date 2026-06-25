# Left-half pin mapping (probe sheet)

> **STATUS: VERIFIED ✓** — both halves flashed; every key types the correct
> character. The current `.dtsi` + overlays match the physical wiring on every
> pin, so no config changes are needed. The "config expects" pins below are the
> confirmed working values.

The matrix is `col2row`. Each **column** shares one controller pin down the whole
column; each **row** shares one pin across the whole row. So you only need to probe
**one key per line** — pressing the key shorts that column pin to that row pin
(through the diode).

Pin notation matches the Supermini silkscreen: `024` = P0.24, `100` = P1.00, etc.

## Key → matrix position (left half)

Pin names shown on the grid are the confirmed working values (all verified by
flashing and typing).

```
 col:      0        1        2        3        4        5
 pin:    P0.22    P0.24    P1.00    P0.11    P1.04    P1.06
        +--------+--------+--------+--------+--------+--------+
 row0   | ESC    | F1     | F2     | F3     | F4     | F5     |  P0.02
 row1   | TAB    | Q      | W      | F      | P      | G      |  P1.15
 row2   | CAPS   | A      | R      | S      | T      | D      |  P1.13
 row3   | LGSP   | Z      | X      | C      | V      | B      |  P1.11
        +--------+--------+--------+--------+--------+--------+
 row4                     | F11    | F12    | BSPC   | SPC    |  P0.10  (thumb)
 row5                     | INS    | LGUI   | DEL    | LALT   |  P0.09  (thumb)
```

`LGSP` = `LG(SPACE)`, `SPC` = `mt LSHFT SPACE`.
Note the thumb keys only use columns 2–5 (rows 4 and 5 have no col‑0/col‑1 keys).

The "config expects" column is what the current config maps each line to:
`pro_micro <N>` is the nexus number in the `.dtsi`/overlay, and `Pphys` is the
physical Supermini silkscreen pin it resolves to on a nice!nano v2 pinout.

## Column pins  (go in `dactyl-manuform-5x6_left.overlay`, col-gpios order 0→5)

Press any one key in the column, read the shared pin:

- col 0 — `TAB`  → config: `pro_micro 4` = P0.22 → **verified ✓** (types correctly)
- col 1 — `Q`    → config: `pro_micro 5` = P0.24 → **verified ✓**
- col 2 — `W`    → config: `pro_micro 6` = P1.00 → **verified ✓**
- col 3 — `F`    → config: `pro_micro 7` = P0.11 → **verified ✓** (earlier P1.11 note was a transcription slip)
- col 4 — `P`    → config: `pro_micro 8` = P1.04 → **verified ✓**
- col 5 — `G`    → config: `pro_micro 9` = P1.06 → **verified ✓**

## Row pins  (go in `dactyl-manuform-5x6.dtsi`, row-gpios order 0→5; shared by both halves)

Press any one key in the row, read the shared pin:

- row 0 — `F1`   → config: `pro_micro 19` = P0.02 → **verified ✓**
- row 1 — `Q`    → config: `pro_micro 18` = P1.15 → **verified ✓**
- row 2 — `A`    → config: `pro_micro 15` = P1.13 → **verified ✓**
- row 3 — `X`    → config: `pro_micro 14` = P1.11 → **verified ✓**
- row 4 — `F11`  → config: `pro_micro 16` = P0.10 → **verified ✓**   (thumb)
- row 5 — `INS`  → config: `pro_micro 10` = P0.09 → **verified ✓**   (thumb)

---

# Right half

The right half is a **separate controller** with its own `col-gpios`
(`dactyl-manuform-5x6_right.overlay`). Two quirks vs. the left:

- The transform applies `col-offset = <6>` and reverses the column order, so the
  **col-gpios index** (the order you edit in the overlay, 0→5) runs **right-to-left**
  across the physical keys. Local index 0 = the **rightmost** key column (`GRAVE`),
  index 5 = the column nearest the center (`F6`/`J`).
- **Rows are shared** with the left (defined in the `.dtsi`), so the right controller
  must be wired to the **same physical row pins** you found on the left. The row
  values below are "must match left" — confirm them, don't expect new pins.

## Key → matrix position (right half)

Grid shown in physical layout (left→right as you look at the right half). The
`idx` row is the col-gpios order in the overlay; note it counts **down** 5→0.

```
 col-gpios idx:    5        4        3        2        1        0
 pin:            P0.22    P0.24    P1.00    P0.11    P1.04    P1.06
                +--------+--------+--------+--------+--------+--------+
 row0           | F6     | F7     | F8     | F9     | F10    | GRV    |  P0.02
 row1           | J      | L      | U      | Y      | SEMI   | BSLH   |  P1.15
 row2           | H      | N      | E      | I      | O      | SQT    |  P1.13
 row3           | K      | M      | ,      | .      | /      | PSCR   |  P1.11
                +--------+--------+--------+--------+--------+--------+
 row4           | LWR    | RET    | LBKT   | RBKT   |        |        |  P0.10  (thumb)
 row5           | PGUP   | PGDN   | SLCK   | PAUS   |        |        |  P0.09  (thumb)
```

`PSCR` = `lt RSE PSCRN`, `LWR` = `mo LWR`, `PAUS` = `PAUSE_BREAK`.
The right thumb keys use col-gpios index 2–5 (transform cols 8–11).

## Column pins  (go in `dactyl-manuform-5x6_right.overlay`, col-gpios order 0→5)

Press any one key in the column, read the shared pin:

- col 0 — `GRV`  → config: `pro_micro 9` = P1.06 → **verified ✓**
- col 1 — `F10`  → config: `pro_micro 8` = P1.04 → **verified ✓**
- col 2 — `F9`   → config: `pro_micro 7` = P0.11 → **verified ✓**
- col 3 — `F8`   → config: `pro_micro 6` = P1.00 → **verified ✓**
- col 4 — `F7`   → config: `pro_micro 5` = P0.24 → **verified ✓**
- col 5 — `F6`   → config: `pro_micro 4` = P0.22 → **verified ✓**

## Row pins  (shared with the left — must match, defined in `dactyl-manuform-5x6.dtsi`)

Press any one key in the row, read the shared pin; confirm it equals the left value:

- row 0 — `F7`   → P0.02 → **verified ✓**
- row 1 — `J`    → P1.15 → **verified ✓**
- row 2 — `H`    → P1.13 → **verified ✓**
- row 3 — `M`    → P1.11 → **verified ✓**
- row 4 — `LBKT` → P0.10 → **verified ✓**   (thumb)
- row 5 — `SLCK` → P0.09 → **verified ✓**   (thumb)

---

## How to read each probe

Pressing a key connects exactly two pins: its **column pin** and its **row pin**.
- For an untested column, pick a key whose row pin you already know — the *other*
  pin is the new column pin (e.g. press `P`: one side is row‑1 P1.15, the other is col‑4).
- For an untested row, pick a key whose column pin you already know — the *other*
  pin is the new row pin (e.g. press `X`: one side is col‑2 P1.00, the other is row‑3).

Send the filled-in pins and I'll rewrite the `.dtsi` + overlay directly from them.
```
