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

The controller polls the salt module roughly every 18 seconds and the module
answers. Both directions carry a **13-byte payload**.

That cadence is not fixed. While someone is working the salt screen at the panel
the controller polls far harder - sub-second gaps appear in captures - and it
also fires an extra poll immediately after any command. A sniffer should treat
18 seconds as the idle rate, not as a guarantee.

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
| 1 | `salt_test_reading` | Drives three thresholds: **above 9 locks the output level against adjustment**, 15 raises "Test Water & Confirm Level", 20 raises "Level Set To 3". Cleared when the level changes or the prompt is acknowledged. The 10–14 band is a state of its own for which the panel shows no message — the only visible effect is that the level stops responding, which is why this component publishes `swg_level_locked` separately. |
| 2 | **Packed** | Low 2 bits = `condition_code`, high 6 bits = `salinity_index`. |
| 3 | `cartridge_days` | Age in days. 120 (4 months) triggers the replace prompt; acknowledging it snoozes for 7 days. |
| 4 | `cell_state` | Relayed untouched by the controller, but **not static** — see below. |
| 5 | `flags` | bit 0 generating, bit 1 active, bit 2 24-hour boost, bit 3 **self-check running**, bit 5 cartridge-service gate. Bit 4 is treated as a second self-check bit but was never observed set. |
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
present* than to *actively producing right now*. Bit 1 was likewise always set.

**Bit 3 is the self-check** — see the section below, which is the best-observed
part of this frame.

**Confirming the salt prompt clears the reading.** In a capture taken alongside a
video of the panel, pressing "Test Water & Confirm Level" dropped
`salt_test_reading` from 10 to 0 within seconds, and the output level - which had
been stuck - could then be changed from 7 to 8. That is the level lock releasing,
observed end to end.

**Byte 12's high bits cycle, and `0x40` is the state in between.** A week of
capture caught 50 changes, and they are not a simple alternation — the three
states form a chain with `0x40` always in the middle:

```
0x00  <->  0x40  <->  0x80
```

Every transition but one passed through `0x40`, which was entered 24 times
against 14 for `0x00` and 11 for `0x80`. The presence nibble stayed at 1
throughout.

**Every one of those changes coincided with `cell_state` dropping to 0** — 49 of
the 50, the exception being a single direct `0x00` -> `0x80`. So whatever the
high bits track, the cell stops being measured while it happens.

That is a much better fit for **polarity reversal** than the original
observation was: two end states with a neutral one between them, and the cell
de-energised across the switch. It is still an inference — nothing on the bus
names it — but it is now built on 50 events rather than two.

`condition_code` is a 2-bit class: **1** means the summer timer is suppressing
output, **3** means low salt. It is forced to 3 whenever the salinity index reads
zero, before anything else looks at it.

Cartridge presence is worth calling out because it is what the controller's own
replace wizard waits on — the wizard advances when the byte reads 0 (after
"Remove Cartridge Now") and again when it reads 1 (after "Insert New Cartridge").

### The self-check

A self-check starts one of two ways: someone presses the water-test button — on
the panel, or through this component, which sets byte 9 of the next request to 1
— or the module starts one itself, which it does more often than not (see
[below](#most-self-checks-are-the-modules-own)). Either way it answers by moving
exactly two bytes and nothing else:

| Byte | What it does |
|---|---|
| `flags` bit 3 | Sets for the duration of the check, clears when it finishes |
| `cell_state` (byte 4) | Cleared to 0 at the start, then climbs back to its resting value |

Fourteen self-checks were captured over a week, commanded and uncommanded alike,
and they all look the same on the wire:

```
          flags   cell_state
before     0x07           11
+0.3s      0x0F            0     bit 3 sets, cell_state cleared
+12s       0x0F            8     climbing back
+30s       0x07           11     bit 3 clear, settled
```

The absolute values differ between runs because the other flag bits ride along
independently — the panel-initiated check ran with boost off and went `0x03` ->
`0x0B` -> `0x03`. The *delta* is the same every time: bit 3, and nothing else in
that byte.

Timing is coarse: the module only speaks when polled, so every interval above is
rounded to the poll cadence. What can be said is that the check runs for tens of
seconds, not milliseconds, and that `cell_state` finishes recovering at about the
same time bit 3 clears.

Three things this settles.

**Bit 3 is not a lockout.** It was previously labelled that way here, and that
was wrong. Level adjustment is gated on `salt_test_reading` being above 9 —
nothing else — and that field is untouched by the check. What bit 3 actually
does is drive the display: while it is set the panel replaces the salt status
message with "Testing", and the salt screen's own status line reads "Testing
Water".

**The check does not necessarily produce a reading.** `salt_test_reading` stayed
at 0 through all three of the checks started from Home Assistant. Bit 3 set,
`cell_state` was re-measured, bit 3 cleared, and the reading never moved. So the
self-check is a measurement of the *cell*, and the salt reading the panel prompts
about is a separate thing that a strip test supplies. Do not wait on
`swg_salt_test` as a way of telling that a check has finished — watch bit 3.

**Bit 3 is not exclusive to the test button.** Stopping the 24-hour boost cycle
set it too, with the same `cell_state` reset, and starting the boost cycle reset
`cell_state` without setting it. Treat bit 3 as "the cell is being re-measured",
which the water test is the usual but not the only cause of.

### Most self-checks are the module's own

A week of capture caught **14 self-check episodes, and only 5 had a human behind
them** — one started at the panel, four sent from Home Assistant. The other nine
had no command on the bus at all: every poll in those windows was a plain
all-`0xFF` request.

The two kinds are cleanly distinguishable, and it is byte 12 that separates them:

| | Commanded (5) | Uncommanded (9) |
|---|---|---|
| `flags` bit 3 | sets | sets |
| `cell_state` cleared and re-climbs | yes | yes |
| Byte 12's high bits | **unchanged** | **always change** |

Every one of the nine uncommanded checks coincided with a polarity change; not
one of the five commanded ones did. So the module re-measures the cell as part of
reversing polarity, and reports that with the same bit it uses for a water test.

The reverse does not hold. Polarity changed 50 times that week, and only a
minority set bit 3 — so a polarity change does not imply a self-check, only the
other way round.

Timing is irregular: gaps between uncommanded checks ran from 2 to 34 hours, with
no schedule visible. **`swg_testing` will therefore go true on its own, at any
hour, with nobody having pressed anything.** Anything built on it — an automation,
an alert — has to expect that.

### Bytes 4 and 11 are relayed without interpretation

The controller copies both straight from the module's frame into its `1E/03`
summary (offsets 8 and 7) and never looks at them. So their meaning cannot be
recovered from the controller side — only the salt module itself knows.

They behave very differently on the wire, though, and that is worth acting on.

**Byte 11 is static.** It held `0x69` across two capture sets taken months apart,
either side of a cartridge replacement — 746 frames, never once different. That
is the signature of a constant: a model or revision identifier rather than a
measurement.

**Byte 4 is a real measurement**, published here as `swg_cell_state`. What is
known about it:

| | |
|---|---|
| Not the output level | Held steady at 8 across every level from 0 to 10 |
| Not the salinity | Unchanged while the salinity index moved 14 ↔ 15 |
| Re-measured by a water test | Drops to 0 the moment the test starts, then climbs back through intermediate values over roughly 40 seconds |
| Differs across a cartridge change | Rested at **8** with a 132-day cartridge; rests at **11** with a 35-day one |
| Does **not** track cartridge age | See below — a week of capture settles this |

### It is not a wear figure

This page used to read `cell_state` as **cell condition** — a health figure that
declines as the cartridge wears — resting on two data points either side of a
cartridge change. A week of continuous capture does not support it.

Over that week the cartridge aged from 35 to 41 days. Taking the resting value
only, with the measurement windows excluded:

| `cartridge_days` | 35 | 36 | 37 | 38 | 39 | 40 | 41 |
|---|---|---|---|---|---|---|---|
| median `cell_state` | 11 | 11 | 11 | 11 | 11 | 11 | 11 |
| highest seen | 11 | 11 | 11 | 11 | 11 | **13** | 11 |

Flat across every age, and on day 40 it went *up* — to 12 and 13, above anything
recorded before. A wear metric does not climb. The value oscillates between 10
and 11 continuously, so a lone step from 11 to 10 means nothing on its own.

That leaves the two-point age correlation looking like coincidence: salinity also
differed between those captures, and the two cartridges were different physical
cells. What `cell_state` measures is still open — what is settled is that it is
not a simple function of cartridge age, and it should not be presented to anyone
as cell health.

**What it does do reliably** is drop to 0 whenever the cell is being re-measured
or its polarity is changing, then climb back over tens of seconds. That makes it
a good progress indicator for those events and a poor gauge of anything else.

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
| 3 | *(no panel message — level locked, salt test 10–14)* |
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

**"Testing" overrides most of the table.** While `flags` bit 3 is set the panel
throws away the message it just computed and shows "Testing" instead — but not
in every state. The two states that mean the system is not running at all,
"Inactive - System Off" (1) and "Inactive - No Circulation" (8), survive; among
the prompt and wizard states only "Service Required" (19) is displaced. Every
other state in the table gives way to "Testing". This component reproduces that,
and also publishes the bit on its own as `swg_testing`.

**Two states are not reachable from the bus.** The decision also depends on the
pump/flow state and on whether the user has acknowledged a prompt at the panel,
neither of which appears on the wire. So "No Circulation" (8) and the
post-acknowledgement states never appear here. Everything else is faithful.
