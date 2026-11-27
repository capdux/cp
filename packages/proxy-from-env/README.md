[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# proxy-from-env

Zero-allocation HTTP_PROXY / HTTPS_PROXY / ALL_PROXY / NO_PROXY URL routing and wildcard bypass matcher (clean-room MIT implementation of npm proxy-from-env, #303 / 169M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-from-env` ([https://github.com/Rob--W/proxy-from-env](https://github.com/Rob--W/proxy-from-env)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `proxy-from-env` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add proxy-from-env
```

```cp
import {
    ProxyEnv,
    PfeParsedUrl,
    pfeEndsWithIgnoreCase,
    pfeParseUrl,
    shouldBypassProxy,
    getProxyForUrl,
} from "proxy-from-env";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ProxyEnv` | `class` | Fields: `http_proxy: string, https_proxy: string, all_proxy: string, no_proxy: string` · Methods: `empty` |
| `PfeParsedUrl` | `class` | Fields: `scheme: string, host: string, port: u16` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pfeEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | Invokes `pfeEndsWithIgnoreCase` and returns `bool`. |
| `pfeParseUrl` | `(url: string): PfeParsedUrl` | Invokes `pfeParseUrl` and returns `PfeParsedUrl`. |
| `shouldBypassProxy` | `(host: string, port: u16, no_proxy: string): bool` | Invokes `shouldBypassProxy` and returns `bool`. |
| `getProxyForUrl` | `(target_url: string, env: ProxyEnv): string` | Queries/retrieves data via `getProxyForUrl` returning `string`. |

## Minimal Example

```cp
import { getProxyForUrl } from "proxy-from-env";

pub fn main() {
    let result = getProxyForUrl("example", /* env: ProxyEnv */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
