[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `cssesc`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cssesc` ([https://github.com/mathiasbynens/cssesc](https://github.com/mathiasbynens/cssesc))

Zero-heap-allocation CSSOM-compliant CSS identifier and string escaper in pure CP.

## Features

- **CSSOM Spec Compliant**: Correctly escapes leading digits, `-` followed by digit, lone `-`, control characters, NUL (`U+FFFD`), quotes, backslashes, and non-ASCII Unicode code points (including astral plane `U+10000..U+10FFFF`).
- **Flexible Options**: Supports `is_identifier`, `quotes` (`Single` / `Double`), `wrap`, and `escape_everything`.
- **Zero Heap Allocation**: Provides both `cssescBuf(input, opts, out)` and TLS-backed `cssesc(input, opts)`.

## API

```cp
import {
    CssQuoteStyle,
    CssEscOptions,
    defaultCssEscOptions,
    cssescBuf,
    cssesc,
} from "cssesc";
```

## License

MIT
