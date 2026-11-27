# subtitle@1.0.1 (Agent Quickref)

> Zero-heap SRT & WebVTT subtitle parser, timecode formatter, cue time-shifter, and SRT<->VTT converter | License: MIT
> Import: `import { ... } from "subtitle";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SubtitleFormat`

## Functions
- `export fn parseUint(s: string): i64?;`
- `export fn parseTimestampMs(raw: string): i64?;`
- `export fn put2(out: []u8, pos: &mut usize, v: i64);`
- `export fn put3(out: []u8, pos: &mut usize, v: i64);`
- `export fn formatTimestampMs(ms_in: i64, fmt: SubtitleFormat, out: []u8): string;`
- `export fn parseSubtitles(input: string): CueList;`
- `export fn shiftCues(list: &mut CueList, offset_ms: i64);`
- `export fn formatSubtitles(list: CueList, target_fmt: SubtitleFormat, out: []u8): string;`
