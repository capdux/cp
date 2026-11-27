[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `path-slash`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:path-slash` ([https://github.com/rhysd/path-slash](https://github.com/rhysd/path-slash))

Zero-heap bidirectional path separator conversion between POSIX `/` and Windows `\` in pure CP (clean-room counterpart to Rust `path-slash`).

## Features

- **100% Zero Heap Allocation**: Writes converted paths directly into caller-provided `out: []u8`.
- **Bidirectional Conversion**: `toSlash` (`\` → `/`) and `fromSlash` (`/` → `\`), preserving Windows drive letters (`C:\`), UNC prefixes (`\\server\share`), and verbatim prefixes (`\\?\C:\...`).
- **Lossy UTF-8 Conversion**: `toSlashLossy` replaces malformed UTF-8 byte sequences with `U+FFFD` (`\xEF\xBF\xBD`) while converting backslashes to `/`.
- **Slash Joining & Inspection**: `joinSlash`, `hasBackslash`, `hasForwardSlash`, and `trimTrailingSlashes`.

## Quick Start

```cp
import { toSlash, fromSlash, joinSlash } from "path-slash";

let buf: [128]u8 = undefined;
const n1 = toSlash("C:\\Users\\dev\\file.cp", buf[0..]);     // "C:/Users/dev/file.cp"
const n2 = fromSlash("//server/share/docs/a.md", buf[0..]);  // "\\\\server\\share\\docs\\a.md"
const n3 = joinSlash("src\\core\\", "net\\tcp.cp", buf[0..]); // "src/core/net/tcp.cp"
```

## License

MIT
