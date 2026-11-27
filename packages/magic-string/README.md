[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`magic-string`](https://github.com/Rich-Harris/magic-string)（`MIT`）（包名 `magic-string`）。

# `magic-string`

Zero-heap-allocation original-offset source string editor in pure CP (clean-room MIT implementation inspired by npm `magic-string`).

## Features

- **`MagicString.new(original)`** — Wraps a source string in a fixed-capacity ordered chunk table indexed by original byte offsets.
- **`overwrite(start, end, content)` / `remove(start, end)`** — Replaces or deletes original ranges without shifting subsequent original indices.
- **`prepend(s)` / `append(s)` / `prependLeft(index, content)` / `appendRight(index, content)`** — Inserts text at string ends or precisely to the left/right of any original offset.
- **`hasChanged()` / `toString(out)` / `slice(start, end, out)`** — Renders full or sliced output directly into caller-owned buffers with zero heap allocation.

## Usage

```cp
import { MAGIC_STRING_MAX_CHUNKS, MagicString } from "magic-string";
```

## License

MIT
