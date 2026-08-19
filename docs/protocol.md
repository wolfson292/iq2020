# IQ2020 RS-485 protocol

The IQ2020 is the spa control system used in Caldera and Hot Spring spas. Its
topside panel, salt system, stereo and other accessories all share one RS-485
bus at **38400 8N1**.

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
| `0x1F` | Topside panel — the address this component emulates |
| `0x21` | CoolZone |
| `0x24` | ACE salt system |
| `0x29` | FreshWater salt system |
| `0x33` | Music module |
| `0xFF` | Broadcast |

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
| `02/41` | Undocumented |
| `02/4C` | Get/set clock |
| `02/50` | Undocumented |
| `02/55` | Status block — see [status-packet-0255.md](status-packet-0255.md) |
| `02/56` | Extended status block |
| `02/73` | Hangs the controller until its watchdog resets it |
| `0B/01` | Ping |
| `0B/02`–`04` | Jets 1/2/3 |
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

## Confidence

Not everything here is equally solid, and it is worth being explicit about which
is which.

**Well established** — the frame format and checksum (validated against every
captured frame, including a 3-byte CoolZone frame), the framing and inter-frame
gap behaviour, the salt system exchange in both directions, the three `1E/03`
layout variants and which of their offsets are unused, the salt status states,
the `17/02` and `17/05` light layouts, and the 1–7 colour range.

**Inferred, not proven**

- **Light colour names.** The range 1–7 is certain, but the names in
  [lights.md](lights.md) come from watching hardware. Seven names had to be
  fitted to seven slots after "Rainbow" was identified as the colour-cycle flag
  rather than a colour, so the table could be rotated by one.
- **Salinity units.** The index-to-value conversion is exact, but nothing labels
  the result, so this component publishes it without a unit rather than guessing
  at ppm and a scale factor.
- **Cell runtime.** The 24-bit counter in the salt frames is believed to be
  runtime; that name is a guess.

**Unverified** — the command names for groups `0x02` and `0x0B` are inherited
from earlier work on this project and were not all independently confirmed. The
`1D/07` heat pump command is only partly understood; the spa this component was
developed against has no heat pump.

## The other serial link

The controller has a second UART running an unrelated protocol: ASCII magic
`"XMS"`, a 2-byte big-endian length (max `0x32`) and a 2-byte big-endian
checksum. It has nothing to do with the spa bus and was not investigated.

## Related

[Ylianst/ESP-IQ2020](https://github.com/Ylianst/ESP-IQ2020) is an independent
project against the same hardware.
