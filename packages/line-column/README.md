[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `line-column`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:line-column` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:line-index` ([https://github.com/rust-lang/rust-analyzer](https://github.com/rust-lang/rust-analyzer))（原库名 `line-column` → CP 包名 `line-column`）

Zero-heap-allocation byte offset <-> 1-based `(line, col)` locator, zero-copy source line extractor, and LF/CRLF line counter in pure CP.

## Features

- **`offsetToLineCol`**: Converts byte offsets (`0..=text.len`) to 1-based `(line, col)` across LF and CRLF files.
- **`lineColToOffset`**: Inverse mapping from 1-based `(line, col)` back to byte offsets with bounds checking.
- **`getLine`**: Zero-copy extraction of any 1-based line with trailing `\r\n` / `\n` stripped.
- **`countLines`**: Counts logical lines (`""` -> 0, trailing `\n` terminates last line).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { LineCol, offsetToLineCol, lineColToOffset, getLine, countLines } from "line-column";
```

## License

MIT
