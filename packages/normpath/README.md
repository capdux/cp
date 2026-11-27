[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `normpath`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:normpath` ([https://github.com/dylni/normpath](https://github.com/dylni/normpath))

Zero-heap cross-platform lexical path normalization engine in pure CP (clean-room counterpart to Rust `normpath`).

## Features

- **100% Zero Heap Allocation**: Writes normalized paths directly into caller-provided `out: []u8`.
- **POSIX Normalization (`normalizePosix`)**: Collapses repeated `/`, strips `.` segments, resolves `..` without escaping `/` root, preserves leading `..` on relative paths, and normalizes empty paths to `.`.
- **Windows Normalization (`normalizeWindows`)**: Supports drive roots (`C:\`), drive-relative (`C:foo`), UNC (`\\server\share`), verbatim prefixes (`\\?\C:\...`, `\\?\UNC\server\share\...`), and unifies `/` and `\` into `\`.
- **Path Joining & Inspection**: `joinNormalized`, `joinNormalizedWindows`, `hasParentTraversal`, `escapesRoot`, `isNormalized`, `isVerbatimWindows`, and `stripVerbatimPrefix`.

## Quick Start

```cp
import { normalizePosix, normalizeWindows, joinNormalized } from "normpath";

let buf: [128]u8 = undefined;
const n1 = normalizePosix("/a/./b/../c///d/", buf[0..]); // "/a/c/d"
const n2 = normalizeWindows("c:/foo/./bar/../baz", buf[0..]); // "C:\\foo\\baz"
const n3 = joinNormalized("/usr/local", "../bin/cap", buf[0..]); // "/usr/bin/cap"
```

## License

MIT OR Apache-2.0
