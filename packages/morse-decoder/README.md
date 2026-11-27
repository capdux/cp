[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/morse-code

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:morse-decoder` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:morse` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `morse-decoder` → CP 包名 `@cap/morse-code`）

Zero-heap ITU-R M.1677 International Morse Code encoder and decoder in pure CP.

## Features

- `encodeChar(c)` & `decodeSymbol(sym)`: supports `A-Z`/`a-z`, `0-9`, and `. , ? / - ( ) @`
- `encodeText(input, out)`: single-space between letters, `" / "` between words, collapsing consecutive whitespace
- `decodeText(morse, out)`: decodes Morse sequences and `/` word separators into uppercase ASCII text

## Usage

```cp
import { encodeText, decodeText } from "@cap/morse-code";

let buf: [64]u8 = undefined;
const n = encodeText("SOS", buf[0..64]); // "... --- ..."
```
