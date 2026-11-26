# `@cap/anstyle-parse`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:anstyle-parse` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle)) / `crates.io:anstream` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle))

Zero-heap VT100/ANSI escape sequence state-machine parser in pure CP (clean-room counterpart to Rust `anstyle-parse`).

## Features

- **100% Zero Heap Allocation**: Tokenizes Text, CSI (`ESC[...`), OSC (`ESC]...BEL`/`ST`), and ESC sequences into caller-provided slices.
- **SGR / CSI Parameter Decoder**: `parseCsiParams` decodes multi-parameter sequences (`ESC[1;38;5;196m`, truecolor `ESC[38;2;r;g;bm`, and empty default `ESC[m` -> `[0]`).
- **Fast Text Extraction**: `stripAnsiToBuf`, `visibleByteLen`, and `hasAnsi` backed by the same state machine.
- **Truncated-Input Safe**: Gracefully handles incomplete trailing escape sequences without panicking.

## Quick Start

```cp
import { AnsiToken, tokenizeAnsi, parseCsiParams, stripAnsiToBuf, visibleByteLen } from "@cap/anstyle-parse";

let tokens: [16]AnsiToken = undefined;
const count = tokenizeAnsi(styled_str, &tokens);
const visible = visibleByteLen(styled_str);
```

## License

MIT
