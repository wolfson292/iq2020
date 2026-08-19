# Bus captures

Raw RS-485 traffic logged from a live spa by the component's own `readline_()`
debug output. These are what the field maps in these docs were checked against.

| File | Contents |
|---|---|
| `example_data.txt` | General bus traffic, including the controller's `1E/01` polling of the salt module and its replies |
| `iq_swg.txt` | A single `1E/03` salt summary, annotated with an early set of field labels — several of which turned out to be wrong. See [../swg.md](../swg.md) |
| `decoding_details.txt` | Assorted frames used to validate the checksum algorithm |
| `audio_trim.txt` | Music module traffic (`0x33`) |
| `changes.txt` | Diffed status blocks, used to locate fields that vary |

Log line format:

```
readline_ Full Packet <src> -> <dest> Length:<len> Operation:<op> Data:<data> Checksum:<rx> ChecksumCalc:<calc>
```

`Data` includes the command group and command as its first two bytes.
