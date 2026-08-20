# Lights

The spa has **four independently controllable light zones** (0–3), plus a
pseudo-zone **4 meaning "all"**. Each zone carries three independent pieces of
state:

- **intensity** — 0 to a configured maximum (5 in practice)
- **colour** — 1 to 7
- **colour cycle** — on/off, plus a speed of 0 to 3

The colour cycle is a *separate flag*, not a colour. This is worth stating
plainly because it is easy to get wrong from captures alone: earlier notes on
this project modelled "Rainbow" as an eighth colour value, and it is not one.

## Reading state — `17/05`

Request `17 05` with no payload. The response carries a **20-byte payload**:

| Payload | Contents |
|---|---|
| 0 | Master lights on/off |
| 1–4 | Intensity, zones 0–3 |
| 5–8 | Colour-cycle flag, zones 0–3 |
| 9–12 | Cycle speed (0–3), zones 0–3 |
| 13–16 | Colour, zones 0–3 |
| 17 | 1 if any zone has non-zero intensity |
| 18–19 | Always zero |

Add 2 to get the offset into `data[]`, which includes the `17 05` prefix.

Two derived fields are worth understanding, because their names are misleading
if you only see the bytes:

- **Payload 5–8 is derived from the cycle speed**, not the intensity. It is `1`
  if that zone's speed is non-zero and `0` otherwise, so it reports "this zone
  is cycling", not "this zone is lit".
- **Payload 17 is derived from intensity** — set if any zone's intensity is
  non-zero. This is the closest thing to a global "lights are on".

Asking for zone 4 returns zone 0's values, not an aggregate.

## Colour range

Colour is stored per zone and stepped by the controller:

```c
// step up            // step down
if (colour < 7)       if (colour > 1)
    colour++;             colour--;
else                  else
    colour = 1;           colour = 7;
```

So the range reachable *by stepping* is **1 to 7, wrapping at both ends**. Zero
is never produced; it only appears as an uninitialised value.

**But 8 exists.** The panel's colour palette has eight swatches, and selecting
the eighth sets colour 8 and turns the cycle flag on in the same status frame -
observed live, with the cycle speed jumping from 0 to 2 at the same instant. The
stepping commands never reach it, which is why it does not appear in the wrap
logic above. So a value of 8 means "colour cycle", and a parser that assumes
1-7 will not know what to do with it.

This has a practical consequence. Any code that steps toward a target colour by
taking the linear difference will **never converge on 0** — it will step down,
wrap to 7, and try again forever. This component clamps requested colours into
1–7 for exactly that reason.

These names are the manufacturer's own, from the spa manual. The wire values were
matched to them by pressing each swatch and watching the reported value: the
panel's palette is laid out in value order — four swatches on the top row, four
on the bottom — so the mapping is direct.

| Value | Colour |
|---|---|
| 1 | Violet |
| 2 | Blue |
| 3 | Cyan |
| 4 | Green |
| 5 | White |
| 6 | Yellow |
| 7 | Red |
| 8 | Rainbow — the colour cycle |

Worth noting that **7 renders closer to orange than red** on the panel, so don't
be thrown if it looks wrong next to the name — the manual calls it Red.

## Writing state — `17/02`

Payload is `[zone, command, 0x00]`. Zone is 0–3, or 4 for all.

| Command | Effect |
|---|---|
| `0x02` | Intensity down |
| `0x03` | Intensity up |
| `0x04` | Colour down (wraps 1 → 7) |
| `0x05` | Colour up (wraps 7 → 1) |
| `0x06` | Cycle speed down (floors at 0) |
| `0x07` | Cycle speed up (caps at 3) |
| `0x08` | Colour cycle on |
| `0x09` | Colour cycle off |
| `0x10` | All lights off |
| `0x11` | All lights on |
| `0x34` | Unknown, all-lights |
| `0x35` | Unknown, all-lights |

The controller acknowledges every one of these with the same fixed 1-byte
response, `17 02 06`. The acknowledgement tells you nothing about the resulting
state — poll `17/05` for that.

Intensity and speed both saturate rather than wrap, so repeating a step command
past the end of the range is harmless. Colour wraps, so it is not.

Changing colour while a zone is cycling stops the cycle first.
