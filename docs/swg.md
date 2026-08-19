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
| 0 | Output level | 0–10. Anything above 10 is treated as 3. |
| 1 | Salt test reading | Drives the "Test Water" / "Level Set To 3" prompts. Above 9 it also locks the level out of adjustment. |
| 2 | **Packed** | Low 2 bits = status class, high 6 bits = salinity index. |
| 3 | Cartridge age | In days. 120 (4 months) triggers the replace prompt. |
| 4 | Unknown | |
| 5 | Flags | bit 0 generating, bit 1 active, bit 2 24-hour boost, bit 3 test/lockout. |
| 6 | Error code | Shown on the panel as "Error N". |
| 7 | Status (variant B only) | Folded in as `byte7 * 10 + byte6`. |
| 8–10 | Counter | 24-bit **little-endian**. Believed to be cell runtime. |
| 11 | Unknown | |
| 12 | Packed flags | Low nibble, plus bits 6–7 (variant C) or 4–5 (variant B). |

**Byte 2 is two fields, not one.** Decode it as:

```python
status_class = payload[2] & 0x03      # 1 = summer timer, 3 = low salt
salinity_idx = payload[2] >> 2        # 0..63
```

A salinity index of zero forces the status class to 3 (low), before anything
else examines it.

### Salinity

The index is not a reading — it is a table lookup. It converts through a
32-entry table, plus an offset of 249:

```
0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 53, 56, 60, 63, 66,
69, 73, 76, 79, 82, 85, 89, 92, 95, 100, 106, 113, 119, 126, 132, 139
```

An index of 32 or more reads as 0 before the offset. The resulting range is
249–388. **Nothing on the bus labels this value with a unit**, so this component
publishes it without one rather than guessing at ppm and a scale factor. An index
of 24 or above is treated as "high salt".

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
| 9 | 1 | **Start water test** |
| 10 | 1 | Unknown |
| 11 | 1 / 2 / 0x5A | Unknown |
| 2, 3, 12 | | ACE (`0x24`) only; byte 12 is a computed check value |

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
