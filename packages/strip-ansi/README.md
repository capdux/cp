[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# strip-ansi

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-ansi` ([https://github.com/chalk/strip-ansi](https://github.com/chalk/strip-ansi)) / `crates.io:strip-ansi-escapes` ([https://github.com/luser/strip-ansi-escapes](https://github.com/luser/strip-ansi-escapes)) / `npm:string-length` ([https://github.com/sindresorhus/string-length](https://github.com/sindresorhus/string-length))（原库名 `strip-ansi` → CP 包名 `strip-ansi`）

Zero-heap ECMA-48 / ANSI terminal escape sequence stripper (CSI, OSC, 2-byte ESC) and visible character counter in pure CP.

## Features

- `hasAnsi(input)`: O(N) fast-path check for `ESC` (`0x1B`) or C1 CSI (`0x9B`)
- `stripAnsi(input, out)`: strips CSI (`ESC [ ...`), OSC (`ESC ] ... BEL/ST`), DCS/APC, and 2-byte ESC sequences into caller buffer
- `strippedLen(input)`: zero-write exact stripped byte length
- `visibleAsciiCount(input)`: counts visible ASCII / UTF-8 characters after stripping ANSI escapes

## Usage

```cp
import { stripAnsi, hasAnsi } from "strip-ansi";

let buf: [64]u8 = undefined;
const n = stripAnsi("\x1b[38;2;255;0;0mRed\x1b[0m", buf[0..64]);
// buf[0..n] => "Red"
```
