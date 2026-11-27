[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `jsonpointer`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsonpointer` ([https://github.com/janl/node-jsonpointer](https://github.com/janl/node-jsonpointer))（原库名 `jsonpointer` → CP 包名 `jsonpointer`）

Zero-heap-allocation RFC 6901 JSON Pointer implementation in pure CP.

## Features

- **Validation (`isValid`)**: Enforces RFC 6901 leading `'/'` and strict `~0`/`~1` escaping.
- **Escaping (`escapeToken` / `unescapeToken`)**: Exact RFC 6901 `~` <-> `~0`, `/` <-> `~1` conversion without double-unescaping (`~01` -> `~1`).
- **Navigation (`countSegments`, `hasSegment`, `getSegment`, `parentPointer`, `appendSegment`)**: Zero-copy slicing and caller-buffer segment extraction.
- **Strict Array Index (`parseArrayIndex`)**: Supports `"0"`, non-leading-zero indices, and `"-"` (`-2`).
- **Streaming Iterator (`PointerIter`)**: Zero-allocation forward traversal over unescaped tokens.

## API

```cp
import {
    isValid,
    escapeToken,
    unescapeToken,
    countSegments,
    hasSegment,
    getSegment,
    parseArrayIndex,
    appendSegment,
    parentPointer,
    PointerIter,
} from "jsonpointer";
```

## License

MIT
