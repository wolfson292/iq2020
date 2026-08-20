# Capturing bus traffic

Most of what is documented here came from watching the bus for a few hours and
seeing which bytes moved. The component has a capture mode for exactly that.

## Enable it

```yaml
switch:
  - platform: iq2020
    iq2020_id: comp_iq2020
    capture:
      name: "Bus Capture"
```

The switch is **off by default and always restores off**, so a reboot part-way
through a capture cannot leave the device logging every frame forever.

## Run a capture

```bash
esphome logs spa-controller.yaml | tee capture.log
```

Turn the switch on, leave it running, turn it off when done. A few hours is
usually enough to catch the periodic behaviour; longer if you are waiting for
something occasional like a filter cycle or a cartridge prompt.

While it runs, **use the spa**. Static traffic tells you very little — the value
is in bytes that change, so press buttons on the panel, change the setpoint,
cycle the lights, start a water test. Note the wall-clock time when you do each
one; correlating an action against the frames around it is what turns an unknown
byte into a known field.

## What gets logged

One line per frame:

```
IQCAP <millis> <OK|BAD> <whole frame as hex, 0x1C through checksum>
```

Two deliberate choices:

- **Nothing is interpreted on the device.** The raw frame is logged and all
  decoding happens offline, so a capture stays useful as understanding improves
  — you can re-run a new parser over an old capture.
- **Frames that fail their checksum are logged too**, marked `BAD`. Normal
  operation drops those without their payload, which is unhelpful: a bus problem
  or an unknown device is exactly the case you want the bytes for.

## Analyse it

```bash
./tools/parse-capture.py capture.log
```

The report has three parts:

**Conversations** — who talked to whom, and how often. This is how you find out
which devices your spa actually has. A device that is polled but never answers
is not present: on the spa this component was developed against, the controller
sends 100+ frames to the CoolZone address and nothing ever replies.

**Commands** — every group/command seen, flagged if this project does not decode
it yet. That list is the work queue.

**Payload variability** — for each command, which payload bytes actually changed
and what values they took. This is the useful part. Static bytes are structure;
moving bytes carry information. Running it over the captures in
[captures/](captures/) picks out exactly the fields documented in
[swg.md](swg.md) — output level, salinity, cartridge age, the cell state byte,
the flags byte and the runtime counter — and shows the rest sitting still.

A byte that changes when you press a button, and only then, has just been
identified.

## Reading the output

```
  1E/01 from 29(salt (FreshWater))  (190 frames, 15 bytes)
      [ 0]  11 values: 05x77, 07x58, 06x16, 00x7, 01x6, 04x6 (+5 more)
      [ 4]   4 values: 08x178, 00x10, 06x1, 02x1
      [ 5]   4 values: 07x114, 03x68, 0Fx6, 0Bx2
```

Offsets are into the payload, after the two command bytes — the same numbering
the field tables in these docs use. `05x77` means the value `0x05` appeared 77
times.

A byte with many roughly-equal values is usually a reading or a counter. A byte
with two or three heavily-skewed values is usually flags or a mode. In the
example above, `[0]` ranging across 11 values is the output level being stepped
by hand; `[5]` sitting on two values with occasional excursions is the flags
byte, and the excursions are a boost cycle and a water test.
