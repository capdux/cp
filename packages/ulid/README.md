[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ulid

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ulid` ([https://github.com/ulid/javascript](https://github.com/ulid/javascript)) / `npm:ulidx` ([https://github.com/perry-mitchell/ulidx](https://github.com/perry-mitchell/ulidx))

Zero-allocation ULID (Universally Unique Lexicographically Sortable Identifier: 48-bit Unix ms timestamp + 80-bit entropy, Crockford Base32) encoder, decoder, validator, and monotonic incrementer for CP.

## Features

- **Zero heap allocation**: fixed 16-byte `Ulid` struct and caller-supplied 26-byte encoding buffers.
- **Full Crockford Base32 codec**: `encode` (strict uppercase), `decode` (case-insensitive, maps `I`/`L` -> `1`, `O` -> `0`, rejects leading char `> '7'` overflow), and `isValidUlid`.
- **Sorting & Monotonicity**: `fromParts`, `timestampMs`, `compare` (16-byte lexicographical), and same-millisecond 80-bit `increment` with overflow detection.

## Usage

```cp
import { fromParts, encode, decode, increment, timestampMs } from "ulid";

let rand: [10]u8 = undefined;
// fill rand[0..10]
const u = fromParts(1700000000000 as u64, rand);
let buf: [26]u8 = undefined;
const n = encode(u, buf[0..26]);
```
