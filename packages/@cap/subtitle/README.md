# @cap/subtitle

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:subtitle` ([https://github.com/gsantiago/subtitle.js](https://github.com/gsantiago/subtitle.js))

Zero-heap SRT & WebVTT subtitle parser, timecode formatter, cue time-shifter, and lossless SRT ↔ WebVTT converter in pure CP (replaces npm `subtitle` / `srt-parser-2`).

## Install

```bash
cap add @cap/subtitle
```

## Usage

```cp
import { parseSubtitles, shiftCues, formatSubtitles, SubtitleFormat } from "@cap/subtitle";

let list = parseSubtitles("1\n00:00:01,000 --> 00:00:04,500\nHello Capdux!\n");
shiftCues(&list, 1500); // delay by 1.5s

let out: [1024]u8 = undefined;
const vtt = formatSubtitles(list, SubtitleFormat.WebVtt, out[0..1024]);
```

## API

| Function | Description |
| --- | --- |
| `parseTimestampMs(raw)` | Parse `HH:MM:SS,mmm`, `HH:MM:SS.mmm`, or `MM:SS.mmm` into milliseconds |
| `formatTimestampMs(ms, fmt, out)` | Format milliseconds into SRT or WebVTT timestamp |
| `parseSubtitles(input)` | Parse SRT or WebVTT document into zero-copy `CueList` |
| `shiftCues(list, offset_ms)` | Shift all cue timestamps forward/backward (clamped at 0) |
| `formatSubtitles(list, fmt, out)` | Render `CueList` to SRT or WebVTT |
