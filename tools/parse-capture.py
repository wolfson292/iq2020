#!/usr/bin/env python3
"""Analyse an IQ2020 raw bus capture.

Reads an ESPHome log containing IQCAP lines and reports what was on the bus:
who talked to whom, which commands appeared, and - the useful part - which
bytes of each command's payload actually moved.

    esphome logs spa-controller.yaml | tee capture.log
    ./tools/parse-capture.py capture.log

The capture line format is:

    IQCAP <millis> <OK|BAD> <whole frame as hex, 0x1C through checksum>

Nothing is pre-interpreted on the device, so this script is the only thing that
needs to change as understanding improves.
"""

import argparse
import re
import sys
from collections import Counter, defaultdict

CAP = re.compile(r"IQCAP (\d+) (OK|BAD) ([0-9A-Fa-f]+)")

ADDRESSES = {
    0x01: "controller",
    0x1D: "heatpump/music",
    0x1F: "panel",
    0x21: "coolzone",
    0x24: "salt (ACE)",
    0x29: "salt (FreshWater)",
    0x33: "music",
    0xFF: "broadcast",
}

# Commands this project already decodes, so the report can separate "known" from
# "worth a look".
KNOWN = {
    (0x01, 0x00), (0x01, 0x09), (0x02, 0x41), (0x02, 0x4C), (0x02, 0x55),
    (0x02, 0x56), (0x02, 0x73), (0x0B, 0x01), (0x0B, 0x02), (0x0B, 0x03),
    (0x0B, 0x04), (0x0B, 0x07), (0x0B, 0x1C), (0x0B, 0x1D), (0x0B, 0x1E),
    (0x0B, 0x1F), (0x0B, 0x20), (0x0B, 0x27), (0x17, 0x02), (0x17, 0x05),
    (0x19, 0x00), (0x19, 0x01), (0x1D, 0x07), (0x1E, 0x01), (0x1E, 0x02),
    (0x1E, 0x03),
}


class Frame:
    __slots__ = ("ms", "ok", "raw", "dest", "src", "length", "op", "data")

    def __init__(self, ms, ok, raw):
        self.ms, self.ok, self.raw = ms, ok, raw
        self.dest, self.src, self.length, self.op = raw[1], raw[2], raw[3], raw[4]
        self.data = raw[5:-1]

    @property
    def cmd(self):
        return (self.data[0], self.data[1]) if len(self.data) >= 2 else None

    @property
    def direction(self):
        if self.op & 0x80:
            return "resp"
        if self.op & 0x40:
            return "req"
        return "?"


def addr(a):
    return f"{a:02X}({ADDRESSES[a]})" if a in ADDRESSES else f"{a:02X}"


def load(paths):
    frames, bad, seen = [], 0, 0
    for path in paths:
        stream = sys.stdin if path == "-" else open(path, errors="replace")
        for line in stream:
            m = CAP.search(line)
            if not m:
                continue
            seen += 1
            ms, status, hexs = m.groups()
            try:
                raw = bytes.fromhex(hexs)
            except ValueError:
                continue
            if len(raw) < 7:
                continue
            if status == "BAD":
                bad += 1
            frames.append(Frame(int(ms), status == "OK", raw))
    return frames, bad, seen


def report(frames, bad, seen, args):
    if not frames:
        print("No IQCAP lines found. Is the capture switch on?")
        return

    span = (frames[-1].ms - frames[0].ms) / 1000.0
    print(f"{len(frames)} frames over {span/60:.1f} min "
          f"({len(frames)/span if span else 0:.2f}/s), {bad} bad checksum, {seen} lines seen")

    print("\n== conversations ==")
    for (s, d), n in Counter((f.src, f.dest) for f in frames).most_common():
        print(f"  {n:7d}  {addr(s):>22} -> {addr(d)}")

    print("\n== commands ==")
    rows = Counter((f.src, f.dest, f.cmd, f.direction) for f in frames if f.cmd)
    for (s, d, cmd, dirn), n in sorted(rows.items(), key=lambda kv: -kv[1]):
        tag = "" if cmd in KNOWN else "   <-- not decoded"
        print(f"  {n:7d}  {cmd[0]:02X}/{cmd[1]:02X} {dirn:<5} "
              f"{addr(s):>22} -> {addr(d):<22}{tag}")

    # The payoff: for each command, which payload bytes actually vary. Static
    # bytes are structure; varying bytes are where the information is.
    print("\n== payload variability (per command, response frames) ==")
    groups = defaultdict(list)
    for f in frames:
        if f.cmd and f.ok and f.direction == "resp":
            groups[(f.src, f.cmd)].append(f.data)
    for (s, cmd), payloads in sorted(groups.items()):
        if len(payloads) < 2:
            continue
        width = min(len(p) for p in payloads)
        print(f"\n  {cmd[0]:02X}/{cmd[1]:02X} from {addr(s)}  "
              f"({len(payloads)} frames, {width} bytes)")
        for i in range(2, width):
            vals = Counter(p[i] for p in payloads)
            if len(vals) == 1:
                continue
            top = ", ".join(f"{v:02X}x{c}" for v, c in vals.most_common(args.top))
            more = "" if len(vals) <= args.top else f" (+{len(vals)-args.top} more)"
            print(f"      [{i-2:2d}] {len(vals):3d} values: {top}{more}")

    if args.unknown:
        print("\n== sample undecoded frames ==")
        shown = set()
        for f in frames:
            if f.cmd and f.cmd not in KNOWN and f.cmd not in shown:
                shown.add(f.cmd)
                print(f"  {f.cmd[0]:02X}/{f.cmd[1]:02X}  {addr(f.src)} -> {addr(f.dest)}  "
                      f"{f.raw.hex().upper()}")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("logs", nargs="+", help="log file(s), or - for stdin")
    ap.add_argument("--top", type=int, default=6, help="values to show per varying byte")
    ap.add_argument("--no-unknown", dest="unknown", action="store_false",
                    help="skip the undecoded-frame samples")
    args = ap.parse_args()
    frames, bad, seen = load(args.logs)
    frames.sort(key=lambda f: f.ms)
    report(frames, bad, seen, args)


if __name__ == "__main__":
    main()
