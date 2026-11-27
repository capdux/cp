[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hostile

Zero-allocation POSIX/Windows /etc/hosts parser, case-insensitive forward/family/reverse lookup, and canonical formatter (hostile / Rust hostsfile parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hostile` ([https://github.com/feross/hostile](https://github.com/feross/hostile))（原库名 `hostile` → CP 包名 `hostile`）

| Metadata | Value |
| --- | --- |
| Package | `hostile` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hostile
```

```cp
import {
    HostEntry,
    HostsTable,
    isBlank,
    parseHosts,
    lookupIp,
    lookupIpByFamily,
    reverseLookup,
    pushByte,
} from "hostile";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HostEntry` | `class` | Fields: `ip: string, hostname: string, is_ipv6: bool` · Methods: `empty` |
| `HostsTable` | `class` | Fields: `entries: [MAX_HOST_ENTRIES]HostEntry, count: usize` · Methods: `empty, entryAt` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isBlank` | `(c: u8): bool` | Checks predicate `isBlank` and returns `bool`. |
| `parseHosts` | `(content: string): HostsTable` | Parses input via `parseHosts` and returns `HostsTable`. |
| `lookupIp` | `(table: HostsTable, hostname: string): string?` | Queries/retrieves data via `lookupIp` returning `string?`. |
| `lookupIpByFamily` | `(table: HostsTable, hostname: string, ipv6: bool): string?` | Queries/retrieves data via `lookupIpByFamily` returning `string?`. |
| `reverseLookup` | `(table: HostsTable, ip: string): string?` | Invokes `reverseLookup` and returns `string?`. |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `formatHosts` | `(table: HostsTable, out: []u8): string` | Serializes/formats data via `formatHosts` returning `string`. |

## Minimal Example

```cp
import { parseHosts } from "hostile";

pub fn main() {
    let result = parseHosts("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
