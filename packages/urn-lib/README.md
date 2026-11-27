[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/urn-parse`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:urn-lib` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:urn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `urn-lib` → CP 包名 `@cap/urn-parse`）

Zero-heap-allocation RFC 8141 Uniform Resource Name (URN) parser, equivalence comparator, and canonical formatter in pure CP.

## Features

- **`parseUrn`**: Zero-copy parsing of `urn:<nid>:<nss>[?+r][?=q][#f]` with strict RFC 8141 NID validation.
- **`urnEquivalent`**: RFC 8141 §3.1 equivalence comparison (case-insensitive `nid`, exact `nss`, ignoring r/q/f components).
- **`formatUrn`**: Canonical lowercase-scheme & lowercase-NID serialization into caller buffers.
- **`isValidNid`**: Validates 2..32 character NID grammar.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { Urn, isValidNid, parseUrn, urnEquivalent, formatUrn } from "@cap/urn-parse";
```

## License

MIT
