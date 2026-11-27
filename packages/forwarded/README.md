[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# forwarded

Zero-allocation RFC 7239 Forwarded & X-Forwarded-For proxy chain parser with anti-spoofing trusted-hop client IP extraction and private IP detection (forwarded / proxy-addr parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:forwarded-header-value` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `forwarded-header-value` → CP 包名 `forwarded`）

| Metadata | Value |
| --- | --- |
| Package | `forwarded` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add forwarded
```

```cp
import {
    ForwardedHop,
    ForwardedChain,
    unquote,
    stripAddr,
    parseForwardedElement,
    parseForwarded,
    parseXForwardedFor,
    extractClientIp,
} from "forwarded";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ForwardedHop` | `class` | Fields: `for_addr: string, by_addr: string, host: string, proto: string` · Methods: `empty` |
| `ForwardedChain` | `class` | Fields: `hops: [MAX_HOPS]ForwardedHop, count: usize` · Methods: `empty, hopAt, clientIp, clientProto, clientHost, ipAtTrustDepth` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `unquote` | `(raw: string): string` | Invokes `unquote` and returns `string`. |
| `stripAddr` | `(raw: string): string` | Invokes `stripAddr` and returns `string`. |
| `parseForwardedElement` | `(elem: string): ForwardedHop` | Parses input via `parseForwardedElement` and returns `ForwardedHop`. |
| `parseForwarded` | `(header: string): ForwardedChain` | Parses input via `parseForwarded` and returns `ForwardedChain`. |
| `parseXForwardedFor` | `(xff: string, remote_addr: string): ForwardedChain` | Parses input via `parseXForwardedFor` and returns `ForwardedChain`. |
| `extractClientIp` | `(xff: string, remote_addr: string, trusted_hops: usize): string` | Invokes `extractClientIp` and returns `string`. |
| `isPrivateIpv4` | `(s: string): bool` | Checks predicate `isPrivateIpv4` and returns `bool`. |
| `isPrivateIp` | `(ip: string): bool` | Checks predicate `isPrivateIp` and returns `bool`. |

## Minimal Example

```cp
import { parseForwardedElement } from "forwarded";

pub fn main() {
    let result = parseForwardedElement("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
