# Running a decoding session at the spa

Passive capture only shows you bytes that happen to change on their own. Most of
what is still unknown moves only when someone touches the spa, so an hour at the
tub is worth a day of idle logging.

## Before you go out

1. Turn **Bus Capture** on and confirm frames are flowing.
2. Have something to write times on — a phone note is fine.
3. Know that **panel button presses do not appear on the bus.** The topside
   remote is on a separate link (see [protocol.md](protocol.md)), so what you see
   is the *effect* of a press in the controller's next status reply, not the
   press itself. Resolution is one poll interval, so leave a gap between actions.

## The rules that make it work

**Write down the time of every action.** Wall clock to the nearest 5–10 seconds
is enough. Without it a capture is just bytes; with it, every byte that moved in
that window is a candidate. This is the single thing that determines whether the
session was worth doing.

**One thing at a time, then wait.** Leave 20–30 seconds between actions. Two
changes inside one poll interval are indistinguishable afterwards.

**Prefer the panel over Home Assistant** where you have the choice. Commands sent
from this component exercise paths already understood; the panel exercises the
ones that are not.

**Go back to where you started.** Returning each setting to its original value
doubles the evidence — a byte that moves one way and back is far more convincing
than one that moves once.

## Worth doing, in order

### 1. Run a water test on the salt system

The highest-value single action. It is the only thing known to move
`swg_cell_state`, the one live field nobody has named, and it is when the module
emits its frames to the unexplained `0x99` destination.

Note the time you start it, then leave it completely alone until it finishes.
Note the time it ends and anything the panel displayed during it.

### 2. Walk one light through every colour

This settles the largest documented guess in the protocol. The colour range 1–7
is certain; the *names* are fitted from observation and the whole table could be
rotated by one position.

Pick one light. Turn it on, then advance the colour **one press at a time**,
and after each press write down the time and **the colour you actually see**.
Go all the way round until it wraps to where it started.

Do not skip the wrap — confirming it goes 7 → 1 and never shows an eighth colour
is what proves the range.

### 3. Clear the salt level lock

If the panel is showing a salt prompt, acknowledging it should drop the salt test
reading below 10 and let the output level move again. Note the time you
acknowledge, then try changing the level and note whether it responds.

This is the one case where the panel shows *no message* for a real state, so
watching the reading fall is the only way to see the transition.

### 4. Colour cycle and its speed

Turn the colour cycle on for one light, let it run 30 seconds, then step the
speed through each setting, pausing at each. Then turn it off.

### 5. Econ mode and filter times

Toggle econ, wait, toggle it back. If you can reach the filter cycle times,
change one by a known amount and change it back.

### 6. Jets, blower, lights on/off

Each of these moves several still-unidentified bytes in the main status block.
If a jet has more than one speed, step through them rather than just on/off.

### 7. Locks and the summer timer

Toggle each on, wait, toggle it back off.

## What not to bother with

- **The setpoint.** Every change writes non-volatile storage — see
  [protocol.md](protocol.md#settings-persistence--dont-hammer-the-setpoint). One
  change and one change back is fine; do not sweep it.
- **Anything the spa is not fitted with.** The controller answers `0` for
  hardware that is not present, so there is nothing to learn.

## Afterwards

```bash
./tools/parse-capture.py capture.log
```

Then line your noted times up against the *payload variability* section. A byte
that changed only in the window where you did one specific thing has just been
identified — that is the whole method.
