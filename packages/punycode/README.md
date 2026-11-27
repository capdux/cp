[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# punycode

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:punycode` ([https://github.com/mathiasbynens/punycode.js](https://github.com/mathiasbynens/punycode.js)) / `crates.io:punycode` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-allocation RFC 3492 Punycode / IDNA (`xn--...`) encoder and decoder for CP.

## Features

- **Zero heap allocation**: pure CP RFC 3492 Bootstring implementation operating on caller-supplied `[]u8` buffers.
- **Label codec**: `encodeLabel` / `decodeLabel` with strict UTF-8 validation and integer overflow protection.
- **Full domain conversion**: `toAsciiDomain` / `toUnicodeDomain` handling dot-separated labels (`bücher.example.com` <-> `xn--bcher-kva.example.com`).

## Usage

```cp
import { toAsciiDomain, toUnicodeDomain } from "punycode";

let ascii: [128]u8 = undefined;
const n = toAsciiDomain("bücher.example.com", ascii[0..128]);
// "xn--bcher-kva.example.com"
```
