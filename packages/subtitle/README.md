[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# subtitle

Zero-heap SRT & WebVTT subtitle parser, timecode formatter, cue time-shifter, and SRT<->VTT converter

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:subtitle` ([https://github.com/gsantiago/subtitle.js](https://github.com/gsantiago/subtitle.js))

| Metadata | Value |
| --- | --- |
| Package | `subtitle` |
| Version | `1.0.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add subtitle
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SubtitleFormat` | `Srt, WebVtt` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Cue` | `class` | Fields: `index: u32, start_ms: i64, end_ms: i64, text: string` |
| `CueList` | `class` | Fields: `cues: [64]Cue, count: usize, format: SubtitleFormat` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseUint` | `(s: string): i64?` | Parses input via `parseUint` and returns `i64?`. |
| `parseTimestampMs` | `(raw: string): i64?` | Parses input via `parseTimestampMs` and returns `i64?`. |
| `put2` | `(out: []u8, pos: &mut usize, v: i64)` | Mutates or appends state via `put2`. |
| `put3` | `(out: []u8, pos: &mut usize, v: i64)` | Mutates or appends state via `put3`. |
| `formatTimestampMs` | `(ms_in: i64, fmt: SubtitleFormat, out: []u8): string` | Serializes/formats data via `formatTimestampMs` returning `string`. |
| `parseSubtitles` | `(input: string): CueList` | Parses input via `parseSubtitles` and returns `CueList`. |
| `shiftCues` | `(list: &mut CueList, offset_ms: i64)` | Executes `shiftCues` in-place. |
| `formatSubtitles` | `(list: CueList, target_fmt: SubtitleFormat, out: []u8): string` | Serializes/formats data via `formatSubtitles` returning `string`. |

## Minimal Example

```cp
import { parseUint } from "subtitle";

pub fn main() {
    let result = parseUint("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
