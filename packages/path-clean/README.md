[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `path-clean`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:path-clean` ([https://github.com/danreeves/path-clean](https://github.com/danreeves/path-clean))（原库名 `path-clean` → CP 包名 `path-clean`）

Zero-heap lexical POSIX path cleaner in pure CP, faithfully following Go `filepath.Clean` and Rust `path-clean` semantics: collapses redundant slashes, removes `.` segments, resolves `a/..`, clamps excess `..` at `/` for absolute paths, preserves leading `..` for relative paths, strips trailing `/` (except root `/`), and normalizes empty paths to `"."`.

零堆分配纯 CP 词法路径清洗库，完整对标 `crates.io:path-clean` 与 Go `filepath.Clean` 规则：折叠多余 `/`、消除 `.`、解析 `a/..`、绝对路径多余 `..` 钳位在 `/`、保留相对路径前导 `..`、去除尾部 `/`（根路径 `/` 除外）、空路径归一化为 `"."`。

## Installation

```bash
cap add path-clean
```

## API

| Function | Signature | Description |
| --- | --- | --- |
| `cleanPath` | `(path: string, out: []u8): usize` | Lexically clean a path into `out`, returning bytes written |
| `cleanJoin` | `(a: string, b: string, out: []u8): usize` | Join two paths and lexically clean the result into `out` |
| `isClean` | `(path: string): bool` | Check in O(N) zero-allocation whether `path` is already clean |

## License

MIT OR Apache-2.0
