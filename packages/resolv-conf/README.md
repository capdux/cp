[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# resolv-conf

Zero-heap POSIX /etc/resolv.conf DNS resolver configuration parser

| Metadata | Value |
| --- | --- |
| Package | `resolv-conf` |
| Version | `0.7.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add resolv-conf
```

```cp
import { ResolvConfig, defaultResolvConfig, parseResolvConf } from "resolv-conf";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ResolvConfig` | `class` | Fields: `nameservers: [8]string, nameserver_count: usize, search_domains: [8]string, search_count: usize, domain: string, ndots: u32, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `defaultResolvConfig` | `(): ResolvConfig` | Invokes `defaultResolvConfig` and returns `ResolvConfig`. |
| `parseResolvConf` | `(content: string): ResolvConfig` | Parses input via `parseResolvConf` and returns `ResolvConfig`. |

## Minimal Example

```cp
import { parseResolvConf } from "resolv-conf";

pub fn main() {
    let result = parseResolvConf("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
