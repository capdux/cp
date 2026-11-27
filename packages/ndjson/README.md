[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ndjson

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ndjson` ([https://github.com/ndjson/ndjson.js](https://github.com/ndjson/ndjson.js))

Zero-heap NDJSON / JSON Lines streaming incremental line splitter, scanner, and writer in pure CP (npm `ndjson` / `split2` parity).

## Features

- `nextLine(src, start_pos, &next_pos)` & `countLines(src)`: zero-copy scanning skipping blank lines and stripping `\r\n` / `\n`
- `NdjsonSplitter`: incremental byte-by-byte streaming splitter with static `[1024]u8` line buffer, `pushByte`, `currentLine`, `flush`, `reset`, and max-line overflow protection
- `writeLine(json_record, dst)`: rejects embedded raw `\n`/`\r` and writes `\n`-terminated NDJSON records

## Usage

```cp
import { NdjsonSplitter, writeLine } from "@cap/ndjson";

let sp = NdjsonSplitter.init();
// feed bytes chunk by chunk; when pushByte returns > 0, read sp.currentLine()
```
