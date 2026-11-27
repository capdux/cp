[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ksuid

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ksuid` ([https://github.com/segmentio/ksuid](https://github.com/segmentio/ksuid)) / `crates.io:svix-ksuid` ([https://github.com/svix/rust-ksuid](https://github.com/svix/rust-ksuid))

Zero-heap Segment KSUID (20-byte K-Sortable Unique IDentifier) 27-character Base62 encoder, decoder, validator, and comparator in pure CP.

## Features

- `KSUID_EPOCH = 1400000000` (`2014-05-13T16:53:20Z`)
- `fromParts(unix_secs, payload)` & `timestampSecs(k)`
- `encode(k, out)`: 160-bit 5-limb division into fixed 27-char zero-padded Base62 (`0-9A-Za-z`)
- `decode(s, &out)` & `isValidKsuid(s)`: strict 27-char Base62 decoding with 160-bit overflow rejection
- `compare(a, b)`: chronological lexicographical byte comparison

## Usage

```cp
import { fromParts, encode, decode, Ksuid } from "@cap/ksuid";

const payload: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
const k = fromParts(1732620000, payload);
let str: [27]u8 = undefined;
const n = encode(k, str[0..27]);
```
