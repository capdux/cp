[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hosted-git-info

Zero-heap parser & URL formatter for GitHub, GitLab, and Bitbucket shorthands, SSH, and HTTPS git URLs (npm hosted-git-info equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hosted-git-info` ([https://github.com/npm/hosted-git-info](https://github.com/npm/hosted-git-info))

| Metadata | Value |
| --- | --- |
| Package | `hosted-git-info` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hosted-git-info
```

```cp
import {
    HostedGitInfo,
    hgiPush,
    hgiStartsWith,
    hgiEndsWith,
    hgiFindChar,
    hgiBuildFromPath,
    fromUrl,
} from "hosted-git-info";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HostedGitInfo` | `class` | Fields: `host_type: u8, domain: string, user: string, project: string, committish: string, valid: bool` · Methods: `invalid, formatHttps, formatSsh, formatBrowse, formatBugs` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hgiPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `hgiPush` and returns `usize`. |
| `hgiStartsWith` | `(s: string, prefix: string): bool` | Invokes `hgiStartsWith` and returns `bool`. |
| `hgiEndsWith` | `(s: string, suffix: string): bool` | Invokes `hgiEndsWith` and returns `bool`. |
| `hgiFindChar` | `(s: string, ch: u8): usize?` | Invokes `hgiFindChar` and returns `usize?`. |
| `hgiBuildFromPath` | `(host_type: u8, domain: string, path_and_hash: string): HostedGitInfo` | Invokes `hgiBuildFromPath` and returns `HostedGitInfo`. |
| `fromUrl` | `(raw: string): HostedGitInfo` | Invokes `fromUrl` and returns `HostedGitInfo`. |

## Minimal Example

```cp
import { hgiPush } from "hosted-git-info";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hgiPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
