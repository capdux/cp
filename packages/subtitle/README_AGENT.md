# subtitle — AI Agent Quick Ref

Zero-heap SRT & WebVTT subtitle parser, timecode formatter, cue time-shifter, and SRT<->VTT converter (`v1.0.1`, `MIT`)

## Import

```cp
import {
    SubtitleFormat,
    Cue,
    CueList,
    parseUint,
    parseTimestampMs,
    put2,
    put3,
    formatTimestampMs,
} from "subtitle";
```

## Core Types & Constants

```cp
enum SubtitleFormat { Srt, WebVtt }
class Cue { index: u32, start_ms: i64, end_ms: i64, text: string }
class CueList { cues: [64]Cue, count: usize, format: SubtitleFormat }
```

## API Signatures

```cp
fn parseUint(s: string): i64?;
fn parseTimestampMs(raw: string): i64?;
fn put2(out: []u8, pos: &mut usize, v: i64);
fn put3(out: []u8, pos: &mut usize, v: i64);
fn formatTimestampMs(ms_in: i64, fmt: SubtitleFormat, out: []u8): string;
fn parseSubtitles(input: string): CueList;
fn shiftCues(list: &mut CueList, offset_ms: i64);
fn formatSubtitles(list: CueList, target_fmt: SubtitleFormat, out: []u8): string;
```

## Minimal Usage

```cp
import { parseUint } from "subtitle";

pub fn main() {
    let result = parseUint("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"subtitle"`; install via `cap add subtitle`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
