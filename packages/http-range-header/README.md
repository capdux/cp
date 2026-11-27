[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/http-range-header`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http-range-header` ([https://github.com/MarcusGrass/http-range-header](https://github.com/MarcusGrass/http-range-header))

Zero-heap RFC 9110 `Range: bytes=...` HTTP header parser, boundary validator, and `Content-Range` formatter in pure CP (clean-room counterpart to Rust `http-range-header`).

## Features

- **100% Zero Heap Allocation**: Parses syntactic byte ranges and validates concrete byte ranges directly into caller-provided slices.
- **Full RFC 9110 Byte Range Forms**: Supports `start-end`, `start-` (open-ended), `-suffix` (suffix-length), and comma-separated multi-range lists while rejecting non-`bytes=` units and malformed syntax.
- **Boundary Clamping & 416 Detection**: `validateRanges` clamps overflowing `end` values to `total_size - 1`, resolves suffix lengths, and returns `-1` when unsatisfiable.
- **Response Formatting**: `formatContentRange` (`bytes start-end/total`) and `formatUnsatisfiableContentRange` (`bytes */total`).

## Quick Start

```cp
import {
    SyntacticByteRange,
    ValidatedByteRange,
    parseRangeHeader,
    validateRanges,
    formatContentRange,
} from "@cap/http-range-header";

let syn: [8]SyntacticByteRange = undefined;
const parsed = parseRangeHeader("bytes=0-499, -200", syn[0..]); // 2

let valid: [8]ValidatedByteRange = undefined;
const count = validateRanges(syn[0..parsed], 1000, valid[0..]); // 2

let hdr: [64]u8 = undefined;
const n = formatContentRange(valid[0].start, valid[0].end, 1000, hdr[0..]); // "bytes 0-499/1000"
```

## License

MIT
