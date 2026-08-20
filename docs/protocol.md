# IQ2020 RS-485 protocol

The IQ2020 is the spa control system used in Caldera and Hot Spring spas. Its
salt system, stereo, internet gateway and other accessories share one RS-485
accessory bus at **38400 8N1**. (The topside remote is *not* on this bus — see
[Addresses](#addresses).)

## Frame format

```
   +------+------+------+------+------+---------------+------+
   | 0x1C | dest | src  | len  |  op  |   data[len]   | cksum|
   +------+------+------+------+------+---------------+------+
```

| Field | Meaning |
|---|---|
| `0x1C` | Start of frame. **Not** included in the checksum. |
| `dest` | Destination address. |
| `src` | Source address. |
| `len` | Number of `data` bytes. `data[0]` is the command group, `data[1]` the command. |
| `op` | Bit 6 (`0x40`) — a response is requested. Bit 7 (`0x80`) — this frame *is* a response. |
| `cksum` | One's complement of the 8-bit sum of `dest` through the last data byte. |

Total frame length on the wire is `len + 6`.

```python
def checksum(dest, src, ln, op, data):
    return (~((dest + src + ln + op + sum(data)) & 0xFF)) & 0xFF
```

## Framing rules

- More than **100 ms of line silence resets the receive index**. That inter-frame
  gap is the only framing signal; there is no end-of-frame delimiter.
- A frame starts at `0x1C`; anything else is discarded.
- `len` must be greater than 1.
- A frame is accepted once `len + 6` bytes have arrived, the checksum matches,
  and `dest` is either the receiver's own address or `0xFF` (broadcast).

### Half-duplex collision detection

The controller reads back every byte it transmits and compares it against what
it sent. A mismatch is treated as a bus collision and aborts the transmission.
If you see the controller retrying for no visible reason, something else drove
the line mid-frame.

### Don't send oversized frames

The controller accepts a payload length up to 253, but its transmit buffer holds
only 251 bytes and a function pointer sits immediately after it. A payload longer
than 250 bytes corrupts that pointer, which the transmit path later calls.
Nothing the controller does itself reaches this, but **do not send `len` > 250**.

## Addresses

| Address | Device |
|---|---|
| `0x01` | IQ2020 controller (the bus master) |
| `0x1D` | Heat pump **or** music module — see below |
| `0x1F` | **Spa Connection Kit** — the address this component emulates |
| `0x21` | CoolZone |
| `0x24` | ACE salt system |
| `0x29` | FreshWater salt system |
| `0x33` | Music module |
| `0xFF` | Broadcast |

Reported by [ESP-IQ2020](https://github.com/Ylianst/ESP-IQ2020) but not seen on
the spa this component was developed against, so unverified here: `0x20` external
lights, `0x32` unknown, `0x37` FreshWater IQ (water chemistry — ORP, chlorine,
pH), `0x38` water clarity sensor.

### `0x1F` is the internet gateway, not the topside remote

Worth being precise about, because it changes how the whole command set reads.

**The topside remote is not on this bus.** The controller drives it over a
separate serial link with its own packet format and a screen/element model —
that is where the panel menus, the service configuration screen and the memory
save/restore screen live. None of it is reachable from RS-485.

`0x1F` is the **Spa Connection Kit**, the manufacturer's own internet gateway.
So the commands in the table above are not a panel protocol at all: they are the
remote-control API built for the manufacturer's cloud product. This component
works by occupying that address.

Two consequences:

- **Only one device can hold `0x1F`.** If the official Connection Kit is
  installed, it and this component will collide. They are distinguishable on the
  bus: requests from `0x1F` that you did not send are the kit.
- It explains why the command set is as complete as it is, and why it includes
  the two operations below — which make no sense for a wall panel and obvious
  sense for a remotely managed gateway.

### `0x1D` is dual-purpose

The controller routes `0x1D` differently depending on how the spa is configured:

```
src 0x33 and audio source == 1   -> music module
src 0x1D and audio source == 0   -> music module
src 0x1D otherwise               -> heat pump
```

So `0x1D` is the music module on some installs and the heat pump on others. This
component assumes music lives at `0x33`; on a spa configured the other way it
will misread `0x1D` traffic.

## Command groups

The controller dispatches on `data[0]` (group) and `data[1]` (command).

| Group | Area |
|---|---|
| `0x01` | Versions, setpoint |
| `0x02` | Status blocks, clock, service |
| `0x0B` | Jets, blower, locks, clean cycle |
| `0x17` | Lights |
| `0x19` | Audio |
| `0x1D` | Heat pump |
| `0x1E` | Salt system |
| `0x21` | CoolZone |

### Known commands

| Group/cmd | Meaning |
|---|---|
| `01/00` | Get versions |
| `01/09` | Set temperature |
| `02/41` | Filter cycle times and econ — see below |
| `02/4C` | Get/set clock |
| `02/50` | Diagnostic toggle; only acts when `payload[0] == 6` |
| `02/55` | Status block — see [status-packet-0255.md](status-packet-0255.md) |
| `02/56` | Extended status block |
| `02/73` | Remote reset — see below |
| `0B/01` | Ping |
| `0B/02`–`04` | Jets 1/2/3 — see below |
| `0B/07` | Blower |
| `0B/1C` | Summer timer |
| `0B/1D` | Spa lock |
| `0B/1E` | Temperature lock |
| `0B/1F` | Clean cycle |
| `0B/20` | All jets |
| `0B/27` | All lights |
| `17/02` | Set lights — see [lights.md](lights.md) |
| `17/05` | Get light status — see [lights.md](lights.md) |
| `19/00` | Set audio |
| `19/01` | Get audio |
| `1D/07` | Get heat pump |
| `1E/02` | Set salt level |
| `1E/03` | Get salt status — see [swg.md](swg.md) |

## The `0x0B` control group

Every command in this group shares one encoding. The payload is a single byte
holding **the desired state plus one**:

| Byte | Meaning |
|---|---|
| `0` | Query — change nothing, just report |
| `1` | Off |
| `2` | On |

Jets extend the same idea to speeds, so the byte is `speed + 1`: `1` is off, `2`
is speed 1, `3` is speed 2. Jets 1, 2 and 3 are commands `0x02`, `0x03`, `0x04`.

**Every reply is a single byte carrying the state the controller settled on**,
which is worth parsing rather than assuming the command took:

- A single-speed pump silently promotes speed 1 to speed 2.
- A jet or blower the spa is not configured for answers `0` no matter what you
  send, so an absent device reports itself as off rather than erroring.
- The summer timer answers `0` if the feature is disabled for the spa.

The locks (`0x1D` spa lock, `0x1E` temperature lock) use plain `1` = unlocked,
`2` = locked, and echo the resulting state.

## Filter cycles and econ — `02/41`

Reads and writes the two filter cycle times plus an econ flag. Both request and
response are 5 bytes:

| Byte | Meaning |
|---|---|
| 0–1 | Filter time 1, 16-bit little-endian |
| 2–3 | Filter time 2, 16-bit little-endian |
| 4 | Flags |

On the **request**, the flags byte gates what gets committed: bit 7 writes the
two times, bit 6 writes econ from bit 0. Sent with a flags byte of `0x00`,
nothing is written — it is a pure read.

On the **response**, the flags byte reports bit 0 = econ, bit 1 = circulation.
The two times also appear in the `02/55` status block at offsets 118 and 120,
but econ and circulation appear nowhere else.

## Heat pump — `1D/07`

Present in the controller but not implemented here; the spa this component was
developed against has no heat pump. The request's first payload byte is a mode,
accepted only if `< 5`; the response is two bytes, `[mode, value]`.

## Two operations that only make sense for a gateway

### `02/73` — remote reset

Payload must be exactly `34 87 E5`. On a match the controller enters an
unconditional infinite loop and the watchdog resets it a moment later.

It is a magic-key reboot: a three-byte constant guards it, so it cannot be hit
by accident or by a malformed frame. That is the signature of a deliberate
recovery hook — how an internet-connected gateway reboots a wedged controller
without sending someone out to the spa.

Note what it is not: there is no authentication beyond the constant, and the
constant is public now. Anything on the bus can reset the controller.

### `02/50` — transmit nudge

Payload byte 0 must be `6`. It sets a flag the controller's transmit loop
checks: once ~300 ms have passed it sends whatever is queued **and resets its
retry counter**, instead of waiting for its normal bus-idle condition.

So it is bus arbitration, not diagnostics — a way for whoever holds `0x1F` to
say *"I am done talking, go ahead"*, and to clear a backoff if the controller has
been losing arbitration. A wall panel does not need this. A gateway that polls
hard enough to crowd the bus does.

## Settings persistence — don't hammer the setpoint

The controller keeps its settings in non-volatile storage, and several commands
commit a write **every time they are called**, not just when the value changes:

- `01/09` set temperature
- `0B/1C` summer timer, `0B/1D` spa lock, `0B/1E` temperature lock, `0B/20`
- `02/41` — but only when its flags byte actually requests a write

Two things are usefully free:

- **Light commands (`17/02`) do not persist.** Stepping colour, brightness or
  cycle speed costs nothing, which matters because reaching a target colour
  takes up to six commands.
- **`02/41` with a zero flags byte** is a pure read and commits nothing, so it
  is safe to poll.

The practical consequence is `01/09`. An automation that tracks the setpoint to
an outdoor temperature, or a dashboard slider that fires on every drag step,
turns into a stream of writes to a part with a finite endurance rating. Debounce
setpoint changes and only send when the value has actually moved.

## Confidence

Not everything here is equally solid, and it is worth being explicit about which
is which.

**Well established** — the frame format and checksum (validated against every
captured frame, including a 3-byte CoolZone frame), the framing and inter-frame
gap behaviour, the salt system exchange in both directions, the three `1E/03`
layout variants and which of their offsets are unused, the salt status states
and their thresholds, the `0x0B` control encoding and its replies, the `02/41`
filter/econ layout, the `17/02` and `17/05` light layouts, and the 1–7 colour
range.

Salinity is also settled, in the negative: **there is no salt concentration on
the bus at all.** The panel draws a marker on a bar, and the value behind it is a
screen coordinate. See [swg.md](swg.md).

**Inferred, not proven**

- **Light colour names.** The range 1–7 is certain, but the names in
  [lights.md](lights.md) come from watching hardware. Seven names had to be
  fitted to seven slots after "Rainbow" was identified as the colour-cycle flag
  rather than a colour, so the table could be rotated by one.
- **Cell runtime.** The 24-bit counter in the salt frames is believed to be
  runtime; that name is a guess.
- **Salt frame bytes 4 and 11.** Relayed by the controller without ever being
  read, so nothing on this side of the bus can name them.

**Unverified** — some command names for group `0x02` are inherited from earlier
work on this project and were not all independently confirmed. The `1D/07` heat
pump command is only partly understood; the spa this component was developed
against has no heat pump.

## The other serial link — the topside remote

The controller has a second UART carrying a different protocol: ASCII magic
`"XMS"`, a 2-byte big-endian length (max `0x32`) and a 2-byte big-endian
checksum.

This is the **topside remote**, and it is a richer interface than the spa bus:
the controller pushes screens and elements to it and receives button events back.
The service configuration menu (31 settings toggles) and the memory save/restore
screen live here, and neither is reachable from RS-485.

It was not investigated beyond identifying it, because it is a separate physical
link — an RS-485 tap on the accessory bus cannot see or drive it.

## Related

[Ylianst/ESP-IQ2020](https://github.com/Ylianst/ESP-IQ2020) is an independent
project against the same hardware.
