# iq2020

An [ESPHome](https://esphome.io) component for **IQ2020** spa controllers — the
control system used in Caldera and Hot Spring spas — plus documentation of the
RS-485 protocol it speaks.

The component emulates a topside panel on the spa's RS-485 bus, so the spa is
controlled the same way the real panel controls it. No cloud service and no
modification to the spa.

## What you get

Temperature and setpoint, heater control as a climate entity, jets, blower,
lights (four zones plus all), the salt system, locks and timers, per-device
runtime counters, per-phase voltage/current/power, and the controller's clock.

## Hardware

An ESP32 and an RS-485 transceiver wired to the spa bus. The bus runs at
**38400 8N1**. The example config uses an
[Unexpected Maker ProS3](https://esp32s3.com/pros3.html) with the transceiver on
GPIO15/16, but nothing depends on that particular board.

> Spa controllers switch mains voltage and heating elements. Wire the
> transceiver to the bus only — do not go near the power side unless you are
> qualified to.

## Installation

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/wolfson292/iq2020
      ref: main
    components: [iq2020]
    # ESPHome caches the clone for a day by default, which will happily build
    # from a stale checkout after you push. Keep this short while tracking main.
    refresh: 1min

uart:
  id: uart_spa
  tx_pin: GPIO16
  rx_pin: GPIO15
  baud_rate: 38400

iq2020:
  uart_id: uart_spa
  id: comp_iq2020
  update_interval: 5s
```

Then add whichever entities you want. [`example/spa-controller.yaml`](example/spa-controller.yaml)
is a complete working configuration.

### Salt system

```yaml
sensor:
  - platform: iq2020
    iq2020_id: comp_iq2020
    swg_output_level:   {name: "Salt Output Level"}
    swg_salinity:       {name: "Salinity"}
    swg_age:            {name: "Cartridge Age"}
    swg_error:          {name: "Salt Error Code"}
    swg_cell_runtime:   {name: "Cell Runtime"}

binary_sensor:
  - platform: iq2020
    swg_generating:     {name: "Salt Generating"}
    swg_boost:          {name: "Salt Boost Cycle"}
    swg_cartridge_due:  {name: "Cartridge Due"}

text_sensor:
  - platform: iq2020
    iq2020_id: comp_iq2020
    swg_status:         {name: "Salt System Status"}

number:
  - platform: iq2020
    iq2020_id: comp_iq2020
    swg_level:          {name: "Salt Level"}
```

`swg_status` reports in the controller's own wording — "24-Hour Boost Cycle On",
"Inactive - High Salt", "Cartridge Reached 4 Months - Replace" and so on.

### Lights

```yaml
light:
  - platform: iq2020
    iq2020_id: comp_iq2020
    light_num: 0
    name: "Underwater"
```

Zones 0–3 are the individual lights; `light_num: 4` is all of them. Colour is
exposed as a light effect (Violet, Blue, Cyan, Green, White, Yellow, Red). The
colour cycle and its speed are separate from colour — see
[docs/lights.md](docs/lights.md).

## Protocol documentation

| | |
|---|---|
| [docs/protocol.md](docs/protocol.md) | Framing, checksum, addressing, command table |
| [docs/swg.md](docs/swg.md) | Salt system — both frame types, all three layout variants |
| [docs/lights.md](docs/lights.md) | Light state model and commands |
| [docs/status-packet-0255.md](docs/status-packet-0255.md) | The `02/55` status block, byte by byte |
| [docs/capturing.md](docs/capturing.md) | Capturing bus traffic and analysing it |
| [docs/captures/](docs/captures/) | Raw bus captures |

The frame is short enough to state here:

```
[0x1C] [dest] [src] [len] [op] [data ...] [cksum]
```

`cksum` is the one's complement of the 8-bit sum from `dest` through the last
data byte; the `0x1C` is excluded. `op` bit 6 requests a response, bit 7 marks
one.

[docs/protocol.md](docs/protocol.md) ends with an explicit note on confidence.
One thing is worth knowing before relying on the docs: the light **colour names**
are fitted to hardware observation rather than read off the bus.

On salinity — the panel shows a **bar, not a ppm figure**, and no salt
concentration exists anywhere on the bus. `swg_salinity` reports the marker's
position along that bar as a percentage. See [docs/swg.md](docs/swg.md).

## Licence

GPL-3.0 — see [LICENSE](LICENSE).
