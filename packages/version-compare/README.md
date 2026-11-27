[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/version-compare`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:version-compare` ([https://github.com/timvisee/version-compare](https://github.com/timvisee/version-compare))

Zero-heap flexible version string comparison engine in pure CP (clean-room counterpart to Rust `version-compare`).

## Features

- **100% Zero Heap Allocation**: Parses and compares arbitrary-length version strings entirely on the stack.
- **Flexible Syntax**: Supports arbitrary segment counts, `.` / `-` / `_` separators, optional `v`/`V` prefix, inline alpha-numeric transitions (`1.0rc2`), and `+build` metadata stripping.
- **Pre-Release & Release Ordering**: Recognizes `dev < alpha < beta < rc < release < post` as well as trailing zero equivalence (`1.0 == 1.0.0`).
- **Ergonomic Helpers**: `compareVersions`, `isLess`, `isEqual`, `isGreater`, and `satisfies(ver, op, target)` supporting `<`, `<=`, `==`, `>=`, `>`, `!=`.

## Quick Start

```cp
import { VersionCmp, compareVersions, isLess, satisfies } from "@cap/version-compare";

const c = compareVersions("v1.2.0-beta.1", "1.2.0-rc.1"); // VersionCmp.Lt
const lt = isLess("1.0.0-alpha", "1.0.0");                 // true
const ok = satisfies("2.1.0", ">=", "2.0");                // true
```

## License

MIT
