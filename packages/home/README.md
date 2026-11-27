[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# home

Zero-heap cross-platform home directory resolver, tilde expand/contract, and XDG config dir builder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:home` ([https://github.com/rust-lang/cargo](https://github.com/rust-lang/cargo)) / `npm:untildify` ([https://github.com/sindresorhus/untildify](https://github.com/sindresorhus/untildify)) / `npm:expand-tilde` ([https://github.com/jonschlinkert/expand-tilde](https://github.com/jonschlinkert/expand-tilde)) / `npm:os-homedir` ([https://github.com/sindresorhus/os-homedir](https://github.com/sindresorhus/os-homedir)) / `crates.io:dirs` ([https://github.com/dirs-dev/dirs-rs](https://github.com/dirs-dev/dirs-rs))（原库名 `home` → CP 包名 `home`）

| Metadata | Value |
| --- | --- |
| Package | `home` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add home
```

```cp
import {
    isSlash,
    writeByte,
    trimTrailingSlashes,
    resolveHomeDir,
    expandTilde,
    contractTilde,
    joinConfigDir,
} from "home";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSlash` | `(ch: u8): bool` | Checks predicate `isSlash` and returns `bool`. |
| `writeByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `writeByte` and returns `usize`. |
| `trimTrailingSlashes` | `(s: string): usize` | Invokes `trimTrailingSlashes` and returns `usize`. |
| `resolveHomeDir` | `(home_env: string, userprofile_env: string, homedrive_env: string, homepath_env: string, is_windows: bool, out: []u8): usize` | Invokes `resolveHomeDir` and returns `usize`. |
| `expandTilde` | `(path: string, home_dir: string, out: []u8): usize` | Invokes `expandTilde` and returns `usize`. |
| `contractTilde` | `(path: string, home_dir: string, out: []u8): usize` | Invokes `contractTilde` and returns `usize`. |
| `joinConfigDir` | `(home_dir: string, xdg_config_home: string, app_name: string, is_windows: bool, out: []u8): usize` | Invokes `joinConfigDir` and returns `usize`. |

## Minimal Example

```cp
import { trimTrailingSlashes } from "home";

pub fn main() {
    let result = trimTrailingSlashes("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
