[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# requires-port

Zero-heap checker determining whether a port number is non-default for a given URL protocol

| Metadata | Value |
| --- | --- |
| Package | `requires-port` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add requires-port
```

```cp
import { rptNormalizeProtocol, defaultPortForProtocol, requiresPort } from "requires-port";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rptNormalizeProtocol` | `(proto: string): string` | Invokes `rptNormalizeProtocol` and returns `string`. |
| `defaultPortForProtocol` | `(protocol: string): u16` | Invokes `defaultPortForProtocol` and returns `u16`. |
| `requiresPort` | `(port: u16, protocol: string): bool` | Invokes `requiresPort` and returns `bool`. |

## Minimal Example

```cp
import { rptNormalizeProtocol } from "requires-port";

pub fn main() {
    let result = rptNormalizeProtocol("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
