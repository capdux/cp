[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/vlq

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@jridgewell/sourcemap-codec` ([https://github.com/jridgewell/sourcemap-codec](https://github.com/jridgewell/sourcemap-codec)) / `crates.io:vlq` ([https://github.com/fitzgen/vlq](https://github.com/fitzgen/vlq)) / `npm:vlq` ([https://github.com/Rich-Harris/vlq](https://github.com/Rich-Harris/vlq))（原库名 `@jridgewell/sourcemap-codec` → CP 包名 `@cap/vlq`）

Zero-heap Source Map v3 Base64 VLQ (Variable-Length Quantity) signed integer sequence encoder and decoder in pure CP.

## Features

- `encodeOne(val, out)` & `decodeOne(input, start)`: exact Source Map v3 VLQ encoding with LSB sign bit and 5-bit continuation chunks
- `encodeSlice(values, out)` & `decodeSlice(input, out)`: segment-level encoding and decoding (e.g. `[0, 0, 0, 0]` <-> `"AAAA"`)
- Strict rejection of invalid Base64 characters, truncated continuation bits, and 64-bit shift overflow

## Usage

```cp
import { encodeSlice, decodeSlice } from "@cap/vlq";

const seg: [4]i64 = [0, 0, 10, 4];
let buf: [32]u8 = undefined;
const n = encodeSlice(seg[0..4], buf[0..32]);
```
