# Salt system (SWG)

The salt water generator is a **separate device on the bus**, not part of the
controller. Two models exist and they sit at different addresses:

| Address | Module | Layout variant |
|---|---|---|
| `0x24` | ACE | B |
| `0x29` | FreshWater | C |

This matters more than it looks: the address the module transmits from is what
selects how the controller decodes it, and in turn which of three different
layouts the controller uses when it summarises the salt system to the panel.

There are two ways to read salt status off the bus, and they are not equally
good.

## 1. The module's own frames — `1E/01` (preferred)

The controller polls the salt module roughly every 10 seconds and the module
answers. Both directions carry a **13-byte payload**.

```
01 -> 29  op 0x40  1E 01 | 05 01 FF FF FF 00 FF 01 FF FF FF FF FF
29 -> 01  op 0x80  1E 01 | 05 00 3C 83 08 07 00 00 0C 19 00 69 01
```

This is the richest and least ambiguous source, and it is what this component
parses.

### Response (module → controller)

| Byte | Field | Notes |
|---|---|---|
| 0 | `output_level` | 0–10. Anything above 10 is treated as 3. |
| 1 | `salt_test_reading` | 15 raises "Test Water & Confirm Level", 20 raises "Level Set To 3". Above 9 it also locks the level out of adjustment. Cleared when the level changes or the prompt is acknowledged. |
| 2 | **Packed** | Low 2 bits = `condition_code`, high 6 bits = `salinity_index`. |
| 3 | `cartridge_days` | Age in days. 120 (4 months) triggers the replace prompt; acknowledging it snoozes for 7 days. |
| 4 | `cell_state` | Relayed untouched by the controller, but **not static** — see below. |
| 5 | `flags` | bit 0 generating, bit 1 active, bit 2 24-hour boost, bit 3 test lockout, bit 5 cartridge-service gate. |
| 6 | `error_code` | Shown on the panel as "Error N". |
| 7 | Status (variant B only) | Folded in as `byte7 * 10 + byte6`. |
| 8–10 | `cell_runtime` | 24-bit **little-endian** counter, in hours — see below. |
| 11 | `passthrough_2` | Relay only — see below. |
| 12 | `cartridge_state` | Low nibble is cartridge presence: 0 = absent, 1 = seated. Bits 6–7 (variant C) or 4–5 (variant B) carry separate flags; they were observed cycling through `0x00`, `0x40` and `0x80`. |

### Counter cadence and the flags byte

Two things the captures settle that the protocol alone does not:

**The counter at bytes 8–10 is in hours, and only counts generation.** It
advanced by one at intervals of 145, 61 and 74 minutes of wall clock — which is
what an hours counter looks like on a cell running at part duty, not a
wall-clock timer.

**Flags bit 0 was set in all 190 captured frames.** It never once cleared, at
output levels from 0 to 10. So treat "generating" as closer to *powered and
present* than to *actively producing right now*; bit 2 (boost) and bit 3 (test)
are the bits observed to actually move. Bit 1 was likewise always set.

`condition_code` is a 2-bit class: **1** means the summer timer is suppressing
output, **3** means low salt. It is forced to 3 whenever the salinity index reads
zero, before anything else looks at it.

Cartridge presence is worth calling out because it is what the controller's own
replace wizard waits on — the wizard advances when the byte reads 0 (after
"Remove Cartridge Now") and again when it reads 1 (after "Insert New Cartridge").

### Bytes 4 and 11 are relayed without interpretation

The controller copies both straight from the module's frame into its `1E/03`
summary (offsets 8 and 7) and never looks at them. So their meaning cannot be
recovered from the controller side — only the salt module itself knows.

They behave very differently on the wire, though, and that is worth acting on:

- **Byte 11 is static.** It held `0x69` across all 190 frames in the capture set.
- **Byte 4 moves.** It takes values 0, 2, 6 and 8, and steps during a water test
  (`8 → 0 → 2 → 8` over seven seconds in one capture). This component publishes
  it raw as `swg_cell_state` so it can be correlated against real behaviour;
  the name says where it comes from, not what it means.

### Frames addressed to `0x99`

The module normally answers the controller at `0x01`, but it also emits frames
addressed to **`0x99`** — seven of them in the capture set, clustered around
water-test activity, with valid checksums and well-formed payloads.

The controller ignores these: its receive path only accepts frames addressed to
its own address or to broadcast. **A sniffer should not.** The state they carry
is exactly as good as the frames sent to `0x01`, and filtering on destination
throws away updates precisely when something interesting is happening. This
component matches `1E/01` from `0x24`/`0x29` regardless of destination.

**Byte 2 is two fields, not one.** Decode it as:

```python
status_class = payload[2] & 0x03      # 1 = summer timer, 3 = low salt
salinity_idx = payload[2] >> 2        # 0..63
```

A salinity index of zero forces the status class to 3 (low), before anything
else examines it.

### Salinity — there is no ppm figure anywhere

This is the field most likely to be misread, so it is worth being precise about
what it is and is not.

**The panel never displays a salt concentration.** There is no `ppm` string
anywhere in the controller, and no numeric salt reading is drawn. What the panel
shows is a **marker on a bar**. The index is converted through a 32-entry table
to get how far along that bar the marker sits:

```
0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 53, 56, 60, 63, 66,
69, 73, 76, 79, 82, 85, 89, 92, 95, 100, 106, 113, 119, 126, 132, 139
```

An index of 32 or more reads as 0. The controller then adds a fixed origin of
249 to turn that into a screen X coordinate and emits two draw commands: a fixed
element at `(249, 124)` and the moving marker at `(249 + table[index], 117)`.

Those are pixel coordinates, in the same parameter slots the home screen uses to
place the water temperature at `(100, 35)` and its "F" suffix at `(385, 110)`.
**So `249 + table[index]` is a screen position, not a measurement** — reporting it
as a salinity value conflates the reading with display geometry.

What is meaningful is `table[index]`, the marker's travel along a 139-unit scale.
This component publishes that as a percentage, which is exactly what the bar
shows. The raw index is published separately.

The table is deliberately non-linear — roughly 5 units of travel per index at the
bottom, 3 through the middle, 6–7 at the top — so the marker barely moves while
salt is in range and swings hard at the extremes. An index of 24 or above is
treated as "high salt", which lands at 95/139 ≈ 68% of the bar.

### Request (controller → module)

The request carries pending commands. Every byte is `0xFF` unless a command is
queued, so a plain poll is all-`0xFF` after the first two fields.

| Byte | Value | Meaning |
|---|---|---|
| 0 | 0–10 | Output level |
| 1 | 0–3 | Spa size |
| 4 | 1 / 2 | **24-hour boost cycle: 1 = start, 2 = stop** |
| 6 | 1 / 2 | Level changed, take the value in byte 0 |
| 8 | 1 / 2 | Cartridge replacement sequence |
| 9 | 1 | **Start water test** — the panel's test button |
| 10 | 1 | Unknown |
| 11 | 1 / 2 / 0x5A | Unknown |
| 2, 3, 12 | | ACE (`0x24`) only; byte 12 is a computed check value |

Two of those bytes are **not** "no change" and must carry live values on every
command: byte 0 is the output level and byte 1 the spa size. The module takes
byte 0 as the level unconditionally, so sending a command with a stale or
invented level silently reprograms the output as a side effect of whatever the
command was actually for.

Commands act on the transition, so send the byte once and revert it to `0xFF` on
the next frame — leaving it set re-issues the command on every poll.

## 2. The controller's summary — `1E/03`

The panel can ask the controller for a salt summary. The response is **18 bytes**,
pre-filled with `0xFF` and then selectively overwritten.

This frame is harder to use correctly, for two reasons.

**It has three layouts**, and nothing in the frame identifies which one you are
looking at. A parser keyed only on length will silently misread some
installations. The variant is still knowable, though: watch the bus for `1E/01`
traffic and note whether it came from `0x24` (variant B) or `0x29` (variant C).
That is what this component does.

**Several offsets are never populated.** They stay at the `0xFF` fill forever.
Treating them as data produces convincing-looking garbage.

### Variant C (FreshWater, `0x29`) — the richest

| Offset | Field |
|---|---|
| 0 | Output level |
| 1 | Spa size |
| 2, 3, 5, 9, 11 | **Unused — always `0xFF`** |
| 4 | Flags |
| 6 | Error code, **low 3 bits only** |
| 7 | Unknown |
| 8 | Unknown |
| 10 | **Packed**: `(salinity_index << 2) \| status_class` |
| 12 | Cartridge age, days |
| 13 | Tri-state: 1 if level locked, else 2 if cartridge due, else 0 |
| 14–16 | 24-bit **little-endian** counter |
| 17 | Separate byte, unrelated to 14–16 |

### Variant B (ACE, `0x24`)

Omits offsets 1, 8, 12 and 17. Offset 13 carries `error_code / 10` instead of the
tri-state.

### Variant A

Sparsest — populates only offsets 0, 1, 4, 6, 7 and 10, and offset 6 carries a
raw internal status word rather than the masked error code.

### Confirmation against a real capture

```
1E 03 | 0A 01 FF FF 07 FF 00 69 00 FF 69 FF 01 00 E0 23 00 40
        0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17
```

All five offsets expected to be unused — 2, 3, 5, 9 and 11 — are exactly `0xFF`.
Decoded: level 10, spa size 1, flags `0x07` (generating + active + boost), no
error, salinity index 26, status class 1, cartridge age 1 day, counter 9184.

This is what invalidated the older field labels that had been guessed from
captures alone — `spa_usage`, `g3_sensor_data`, `ph`, `chlorine test` and
`colorimeter test` do not correspond to any real field.

## Status

The controller reduces all of the above to a single status, and shows it as a
message on the panel. This component reproduces that logic as far as bus-visible
data allows:

| Code | Message |
|---|---|
| 0 | Okay |
| 1 | Inactive - System Off |
| 2 | 24-Hour Boost Cycle On |
| 4 | Test Water & Confirm Level |
| 5 | Level Set To 3 - Test & Adjust |
| 6 | Level Set To 1 - Test & Adjust |
| 7 | Restarting |
| 8 | Inactive - No Circulation |
| 9 | Inactive - Summer Timer On |
| 10 | Inactive - High Salt |
| 11 | High Salt |
| 12 | Inactive - Low Salt |
| 18 | Cartridge Reached 4 Months - Replace |
| 19 | Service Required - Contact Dealer |
| 21 | Timeout Error - Check Salt Cartridge |

The precedence is: system off, then service required, then cartridge due, then
the salt-test prompts, then not-generating, then the status class (summer timer,
low salt), and finally salinity against the high threshold with boost as a
modifier.

Only error codes 1, 2, 4 and 5 escalate to "Service Required"; other non-zero
codes do not.

**Two states are not reachable from the bus.** The decision also depends on the
pump/flow state and on whether the user has acknowledged a prompt at the panel,
neither of which appears on the wire. So "No Circulation" (8) and the
post-acknowledgement states never appear here. Everything else is faithful.
