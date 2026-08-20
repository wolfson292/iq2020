# `02/55` and `02/56` status block

The main status packet, and the single richest frame on the bus. The controller
returns it in response to `02 55` (117 data bytes) or `02 56` (134 data bytes).
The two are **byte-identical up to offset 116**; `02/56` appends the clean-cycle
state, filter times, board temperature and the real-time clock. There is no
reason to poll `02/55` if you can poll `02/56`.

Offsets below are into `data[]`, so offset 0 is `0x02` and offset 1 is `0x55` or
`0x56`. Everything in this document is derived from the controller's own packet
builder and checked against 3,732 captured frames.

## Encoding conventions

Read these first — most misreadings of this packet come from getting one of them
wrong.

- **Multi-byte integers are little-endian**, both the 16-bit and 32-bit ones.
- **Runtime counters are 32-bit seconds.** They are lifetime totals, not
  session totals, and they do not reset.
- **Temperatures are 4-byte ASCII, not numbers.** The controller formats them
  with `printf`: `"%3iF"` in Fahrenheit (`" 99F"`, `"102F"`) or `"%4.1f"` in
  Celsius (`"38.5"`). Parse the text; there is no binary temperature anywhere in
  this frame. Which format you get depends on the unit flag at offset 15 bit 6.
- **The electrical values are floats truncated to integers.** The controller
  holds volts, amps and watts as IEEE floats and converts with a C cast, so the
  fraction is discarded, not rounded. A circulation pump drawing 0.58 A reports
  as `0` amps while still reporting its 71 W correctly. Treat the current fields
  as a coarse indicator and the power fields as the real measurement.
- **Three fields are emitted incorrectly by the controller** — see
  [The duplicated-high-byte defect](#the-duplicated-high-byte-defect).
- **Check the length before indexing.** The two commands differ by 17 bytes and
  a decoder written against `02/56` will read past the end of a `02/55` reply.
  Test the actual payload length rather than the command byte, so a corrupt
  frame that happens to pass the checksum cannot walk off the end either.

## Field map

| Offset | Hex | Size | Field | Notes |
|---|---|---|---|---|
| 0 | 00 | 1 | `0x02` | Command group |
| 1 | 01 | 1 | `0x55` / `0x56` | Command |
| 2 | 02 | 1 | — | Always 0. Written unconditionally; a compatibility pad. |
| 3 | 03 | 1 | **Status flags A** | [bits](#offset-3--status-flags-a) |
| 4 | 04 | 1 | **Status flags B** | [bits](#offset-4--status-flags-b) |
| 5 | 05 | 1 | **Status flags C** | [bits](#offset-5--status-flags-c) |
| 6 | 06 | 1 | `heater_on` | **5 when heating, 0 when not.** Not a 1. |
| 7 | 07 | 1 | — | Always 0. |
| 8 | 08 | 1 | **Accessory flags** | [bits](#offset-8--accessory-flags) |
| 9 | 09 | 1 | **Audio flags** | [bits](#offset-9--audio-flags) |
| 10 | 0A | 1 | — | Always 0. |
| 11 | 0B | 1 | `pump_mode` | Circulation-pump state code — see [below](#offset-11--pump-mode) |
| 12 | 0C | 1 | `panel_type` | Topside panel model code. **The same byte is the last byte of the `01/00` version reply.** 0 until the panel reports in. |
| 13 | 0D | 1 | **Config flags A** | [bits](#offset-13--config-flags-a) |
| 14 | 0E | 1 | `install_code` | Enumerated install type — see [below](#offset-14--install-code) |
| 15 | 0F | 1 | **Config flags B** | [bits](#offset-15--config-flags-b). **Bit 6 is the °C/°F flag** and you need it to parse the temperature strings. |
| 16 | 10 | 1 | **Device-absent flags** | Low 4 bits, [inverted](#offset-16--device-absent-flags) |
| 17 | 11 | 2 | `jets1_timeout` | Seconds. One of 900 / 1800 / 3600 / 7200. |
| 19 | 13 | 2 | `jets2_timeout` | Always a copy of `jets1_timeout` |
| 21 | 15 | 2 | `jets3_timeout` | Always a copy of `jets1_timeout` |
| 23 | 17 | 2 | `blower_timeout` | Seconds (a configured minute count × 60) |
| 25 | 19 | 2 | `lights_timeout` | Seconds (a configured minute count × 60) |
| 27 | 1B | 1 | `jets1_speed` | 0 off, 1 low, 2 high |
| 28 | 1C | 1 | `jets2_speed` | |
| 29 | 1D | 1 | `jets3_speed` | |
| 30 | 1E | 1 | `blower_speed` | |
| 31 | 1F | 4 | `high_limit_temp` | ASCII |
| 35 | 23 | 4 | `heater_seconds` | Lifetime heater runtime |
| 39 | 27 | 4 | `jets1_seconds` | Lifetime jets-1 **high speed** runtime |
| 43 | 2B | 4 | `lifetime_seconds` | **Total time the controller has been powered.** Previously labelled "Unknown D". |
| 47 | 2F | 4 | `lost_line_counter` | **Count of mains interruptions.** Previously labelled "Unknown E". A genuinely useful diagnostic: it counts power blips the spa rode through. |
| 51 | 33 | 1 | `summer_timer` | 0/1 |
| 52 | 34 | 1 | `spa_lock` | 0/1 |
| 53 | 35 | 1 | `temp_lock` | 0/1 |
| 54 | 36 | 1 | `clean_cycle_on` | 0/1 |
| 55 | 37 | 4 | `jets2_seconds` | Lifetime jets-2 high speed |
| 59 | 3B | 4 | `jets3_seconds` | Lifetime jets-3 |
| 63 | 3F | 4 | `blower_seconds` | **Lifetime blower runtime.** Previously labelled "Unknown A". |
| 67 | 43 | 4 | `lights_seconds` | Lifetime lights runtime |
| 71 | 47 | 1 | *unidentified* | Single byte, observed constant at 2. Read from a variable this firmware never writes. |
| 72 | 48 | 1 | — | Always 0. |
| 73 | 49 | 4 | `pump_seconds` | **Circulation-pump runtime.** Previously labelled "Total Runtime Seconds" — it is not; offset 43 is the total. On a spa with a 24/7 circ pump the two track within a few minutes, which is why they were confused. |
| 77 | 4D | 4 | `jets1_low_seconds` | Lifetime jets-1 **low speed** runtime |
| 81 | 51 | 4 | `jets2_low_seconds` | Lifetime jets-2 low speed runtime |
| 85 | 55 | 4 | `set_temp` | ASCII |
| 89 | 59 | 4 | `water_temp` | ASCII |
| 93 | 5D | 2 | `baseline_voltage` | Volts. The leg the baseline load is measured against. 120 on the reference spa. |
| 95 | 5F | 2 | `jets_blower_voltage` | Volts. The leg the switched load is measured against. |
| 97 | 61 | 2 | `aux_voltage` | Volts |
| 99 | 63 | 2 | `heater_voltage` | Volts — **defective, see below** |
| 101 | 65 | 2 | `jets_blower_current` | Amps, truncated. Load **above** the baseline: jets and blower. |
| 103 | 67 | 2 | `baseline_current` | Amps, truncated. The idle draw, sampled while no jets and no blower are running. |
| 105 | 69 | 2 | `aux_current` | Amps, truncated. Zeroed below 0.2 A. |
| 107 | 6B | 2 | `heater_current` | Amps — **defective, see below**. Zeroed when `heater_voltage` is at or below 90 V. |
| 109 | 6D | 2 | `jets_blower_power` | Watts — `jets_blower_voltage × jets_blower_current × pf` |
| 111 | 6F | 2 | `baseline_power` | Watts — `baseline_voltage × baseline_current × pf`. **71 W on the reference spa: the circulation pump.** |
| 113 | 71 | 2 | `aux_power` | Watts — `aux_voltage × aux_current` (no power-factor term) |
| 115 | 73 | 2 | `heater_power` | Watts — **defective, see below** |
| — | — | — | **`02/55` ends here** | The frame is 117 data bytes; everything below is `02/56` only. |
| 117 | 75 | 1 | `clean_cycle_state` | 0 idle, 1 running, 2 running (second phase) |
| 118 | 76 | 2 | `filter_time_1` | Minutes. Same value as `02/41` offset 0. |
| 120 | 78 | 2 | `filter_time_2` | Minutes. Same value as `02/41` offset 2. |
| 122 | 7A | 1 | `econ_circ` | bit 0 econ, bit 1 circulation. Same as the `02/41` response flags. |
| 123 | 7B | 1 | `pcb_temp` | **Board temperature in °F**, one byte. Not affected by the °C/°F flag. The controller trips at 180. |
| 124 | 7C | 2 | `periph_current` | **Milliamps** — a current reading multiplied by 1000 before truncation, so this one keeps its precision. |
| 126 | 7E | 1 | `rtc_seconds` | |
| 127 | 7F | 1 | `rtc_minutes` | |
| 128 | 80 | 1 | `rtc_hours` | |
| 129 | 81 | 1 | `rtc_days` | |
| 130 | 82 | 1 | `rtc_months` | |
| 131 | 83 | 2 | `rtc_years` | Little-endian, full year (e.g. 2026 = `D1 07`) |
| 133 | 85 | 1 | `rtc_status` | 1 when the clock is set and running |

## The bitfields

### Offset 3 — status flags A

| Bit | Meaning |
|---|---|
| 0 | Composite not-ready flag. Set when the water-temperature reading is unusable, when the install state is 6, when the board temperature exceeds 180 °F, or when a configuration byte is 0 or 1. |
| 1 | Install state is 6 |
| 2 | A monitored temperature is outside its 40–70 band |
| 3 | **Flow switch closed** — water is moving. Debounced over 15 s, so it lags the pump by up to that long. |
| 4 | An accessory-present check failed |
| 5 | A configuration byte is 0 |
| 6 | **Water temperature unavailable** — sensor fault. When this is set the temperature strings are not trustworthy. |
| 7 | Unused |

### Offset 4 — status flags B

| Bit | Meaning |
|---|---|
| 0 | Temperature lock |
| 1 | Spa lock |
| 2 | **Jets 1 at high speed** |
| 3 | **Jets 2 at high speed** |
| 4 | **Clean cycle running** |
| 5 | Summer timer on |
| 6 | Salt system healthy and generating |
| 7 | Unused |

Bits 2 and 3 are easy to mistake for something else: this byte mixes lock state,
jet speed and the clean cycle. The clean cycle is **bit 4**, not bit 2.

### Offset 5 — status flags C

| Bit | Meaning |
|---|---|
| 0 | Jets 1 at low speed |
| 1 | Jets 2 at low speed |
| 2 | **Circulation pump running** |
| 3 | Blower running |
| 4–6 | Light intensity, 0–7 |
| 7 | Jets 3 at high speed |

Combine with offset 4 for a full jet picture: bit 0 here plus bit 2 there gives
jets 1 low/high.

### Offset 8 — accessory flags

| Bit | Meaning |
|---|---|
| 0 | Unused |
| 1 | **A salt system has been seen on the bus** |
| 2 | Always set |
| 6 | **A CoolZone chiller has been seen on the bus** |

Both presence bits latch on the first frame received from the device, so they
report what has ever answered, not what answered most recently. Bit 6 is also
what widens the setpoint range down to 50 °F — see `01/09` in
[protocol.md](protocol.md).

### Offset 9 — audio flags

| Bit | Meaning |
|---|---|
| 1 | Audio enabled |
| 2 | A second audio-related device is present |

### Offset 11 — pump mode

A state code from the circulation-pump scheduler, not a bitfield, though the
scheduler does rewrite parts of it in place. Observed values:

| Value | Meaning |
|---|---|
| 2 | Normal |
| 5 | Install state 3, pump not permitted |
| 6 | Install state 2 |
| 10 | Normal, secondary demand flag set |

Values with bit 5 or the low bits rewritten (`… & 0xFC \| 0x20`, `… & 0xF3 \| 0x14`)
also occur when the scheduler is rotating through pump modes.

### Offset 13 — config flags A

| Bit | Meaning |
|---|---|
| 1 | Configuration item A present |
| 3, 4, 5, 6, 7 | Further configuration items present |

These track which optional devices the spa was configured with at the factory.
They are static for a given spa, so the practical use is telling two
installations apart rather than tracking state.

### Offset 14 — install code

Not a bitfield. Encodes two configuration items as one of five constants:

| Value | Condition |
|---|---|
| `0x40` | item A clear, item B == 1 |
| `0x72` | item A clear, item B == 0 |
| `0x60` | item A set, item B == 1, item C == 1 |
| `0xF1` | item A set, item B == 1, item C == 0 |
| `0xF2` | item A set, item B == 0, item C == 1 |
| `0x70` | item A set, item B == 0, item C == 0 |
| `0xFF` | anything else |

### Offset 15 — config flags B

| Bit | Meaning |
|---|---|
| 0 | Config item B is 0 |
| 1 | Config item C is 0 |
| 2 | Clean cycle not configured |
| 4 | Config item D is 0 |
| 6 | **Temperatures are in °C** |
| 7 | Config item E is 0 |

Bit 6 is the one that matters: it selects between the `"%3iF"` and `"%4.1f"`
formats used for every temperature string in this packet.

### Offset 16 — device-absent flags

Four bits, and they are **inverted** — a set bit means the device is *not*
configured:

```
bit0 = !item_1   bit1 = !item_2   bit2 = !item_3   bit3 = !item_4
```

A value of 0 therefore means all four are present, which is what the reference
spa reports.

## The electrical measurements

The controller runs four voltage channels and four current channels, and
multiplies selected pairs into power figures. The pairing is **not** in packet
order, which is the trap here:

| Power field | = voltage field | × current field | × power factor |
|---|---|---|---|
| `jets_blower_power` (109) | `jets_blower_voltage` (95) | `jets_blower_current` (101) | yes |
| `baseline_power` (111) | `baseline_voltage` (93) | `baseline_current` (103) | yes |
| `aux_power` (113) | `aux_voltage` (97) | `aux_current` (105) | no |
| `heater_power` (115) | `heater_voltage` (99) | `heater_current` (107) | yes |

Each power figure is an exponential moving average — `x += reading; x -= x/25`,
reported as `x/25` — so it settles over roughly 25 sample periods rather than
tracking instantaneously. Voltages are a trimmed mean of ten samples with the
minimum and maximum discarded.

The baseline/switched split is worth understanding. The controller samples one
physical current channel and, **whenever no jets and no blower are running**,
latches that reading as the baseline. `jets_blower_current` is then the amount by
which the live reading exceeds that baseline. So `baseline_power` is your
always-on load — the circulation pump, 71 W on the reference spa — and
`jets_blower_power` is what the jets and blower add on top.

The heater channel is identifiable independently of the labels: its current is
forced to zero whenever its own voltage is at or below 90 V, and across 3,732
captured frames in which the heater never ran, all three of its fields were
zero.

### The duplicated-high-byte defect

Three of the sixteen-bit fields — `heater_voltage` (99), `heater_current` (107)
and `heater_power` (115) — are **written incorrectly by the controller**. Where
every other field stores the low byte, shifts, and stores the high byte, these
three extract the high byte and store *it* into both positions:

```
correct:    sb lo ; sra 8 ; sb hi        -> [lo, hi]
these three: ext hi ; sb hi ; sb hi      -> [hi, hi]
```

The low byte is lost. Reading them as a normal little-endian 16-bit value gives
a meaningless number such as `0x1111`. The value you can recover is:

```python
approx = data[offset] * 256      # true value is in [approx, approx + 255]
```

So the heater channel is readable, at 256-unit resolution, and only via the
first byte. All three read as zero whenever the heater is off, which is why this
went unnoticed: it only shows up while the heater is drawing power.

This is a defect in the controller, not in any decoder. Nothing can be done
about it from the bus side.

## Worked example

A real `02/56` response, decoded with the map above:

```
offset  3: 08        flow switch closed
offset  5: 04        circulation pump running
offset  6: 00        heater off
offset 12: 06        panel type 6
offset 15: 13        °F (bit 6 clear)
offset 17: 20 1C     jets timeout 7200 s
offset 31: "102F"    high limit
offset 43: 117488278 lifetime 1359.8 days
offset 47: 76        76 mains interruptions
offset 73: 117487640 circ pump 1359.8 days
offset 85: " 99F"    setpoint
offset 89: "102F"    water
offset 93: 120       line voltage
offset 103: 0        baseline current (0.58 A, truncated)
offset 111: 71       baseline power - the circulation pump
offset 118: 60 / 30  filter cycles, minutes
offset 123: 113      board 113 °F
offset 124: 1174     peripheral current 1174 mA
offset 126: 2026-08-18 17:09:29, clock valid
```

## What is still unidentified

Only three things in the whole packet:

- Offset 71, one byte, constant 2. The controller reads it from a variable
  nothing in its own firmware ever writes.
- The individual configuration items behind offsets 13, 14, 15 and 16. Their
  *structure* is known exactly — which bit comes from which item — but what each
  item is called in the manufacturer's configuration menu is not, because that
  menu lives on the topside panel's separate serial link.
- Which physical circuit `aux_voltage` / `aux_current` / `aux_power` measures. It has
  no power-factor term, which the other channels do.

Everything else in the 134 bytes is accounted for.
