[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# robots-parser

Zero-allocation RFC 9309 robots.txt parser and URL allow/disallow evaluator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:robots-parser` ([https://github.com/samclarke/robots-parser](https://github.com/samclarke/robots-parser))（原库名 `robots-parser` → CP 包名 `robots-parser`）

| Metadata | Value |
| --- | --- |
| Package | `robots-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add robots-parser
```

```cp
import {
    isSpaceOrTab,
    trimSpaces,
    startsWithIgnoreCase,
    stripBom,
    cleanLine,
    findColon,
    patMatches,
    parseDelaySeconds,
} from "robots-parser";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSpaceOrTab` | `(c: u8): bool` | Checks predicate `isSpaceOrTab` and returns `bool`. |
| `trimSpaces` | `(s: &[]u8): &[]u8` | Invokes `trimSpaces` and returns `&[]u8`. |
| `startsWithIgnoreCase` | `(haystack: &[]u8, prefix: &[]u8): bool` | Invokes `startsWithIgnoreCase` and returns `bool`. |
| `stripBom` | `(src: &[]u8): &[]u8` | Invokes `stripBom` and returns `&[]u8`. |
| `cleanLine` | `(raw: &[]u8): &[]u8` | Invokes `cleanLine` and returns `&[]u8`. |
| `findColon` | `(line: &[]u8): usize` | Queries/retrieves data via `findColon` returning `usize`. |
| `patMatches` | `(pattern: &[]u8, path: &[]u8): bool` | Invokes `patMatches` and returns `bool`. |
| `parseDelaySeconds` | `(val: &[]u8): i64` | Parses input via `parseDelaySeconds` and returns `i64`. |
| `uaMatchScore` | `(ruleUa: &[]u8, targetUa: &[]u8): i64` | Invokes `uaMatchScore` and returns `i64`. |
| `isAllowed` | `(robots_txt: &[]u8, user_agent: &[]u8, path: &[]u8): bool` | Checks predicate `isAllowed` and returns `bool`. |
| `crawlDelay` | `(robots_txt: &[]u8, user_agent: &[]u8): i64` | Invokes `crawlDelay` and returns `i64`. |
| `firstSitemap` | `(robots_txt: &[]u8): &[]u8` | Invokes `firstSitemap` and returns `&[]u8`. |

## Minimal Example

```cp
import { isSpaceOrTab } from "robots-parser";

pub fn main() {
    let result = isSpaceOrTab(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
