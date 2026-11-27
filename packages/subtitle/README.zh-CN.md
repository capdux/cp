[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# subtitle

> 版本：`1.0.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap SRT & WebVTT subtitle parser, timecode formatter, cue time-shifter, and SRT<->VTT converter

## 安装与导入

```bash
cap add subtitle
```

```cp
import { isDigit } from "subtitle";
```

## 核心导出 API

- `export enum SubtitleFormat`
- `export fn isDigit(c: u8): bool;`
- `export fn parseUint(s: string): i64?;`
- `export fn trimSpaces(s: string): string;`
- `export fn parseTimestampMs(raw: string): i64?;`
- `export fn put2(out: []u8, pos: &mut usize, v: i64);`
- `export fn put3(out: []u8, pos: &mut usize, v: i64);`
- `export fn formatTimestampMs(ms_in: i64, fmt: SubtitleFormat, out: []u8): string;`
- `export fn findArrow(line: string): usize?;`
- `export fn parseSubtitles(input: string): CueList;`
- `export fn shiftCues(list: &mut CueList, offset_ms: i64);`
- `export fn writeStr(out: []u8, pos: &mut usize, s: string);`
- `export fn writeU32(out: []u8, pos: &mut usize, val: u32);`
- `export fn formatSubtitles(list: CueList, target_fmt: SubtitleFormat, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`subtitle@1.0.1`)，许可证：MIT
