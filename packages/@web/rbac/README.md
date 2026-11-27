[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/rbac

Official declarative Role-Based Access Control (RBAC) with wildcard permissions & route guards for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/rbac` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/rbac
```

```cp
import {
    RbacConfig,
    RbacGrant,
    RbacPathRule,
    rbacStrEql,
    rbacStartsWith,
    rbacPermMatches,
    rbacReset,
    rbacGrant,
} from "@web/rbac";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RbacConfig` | `class` | Fields: `role_header: string, default_role: string` |
| `RbacGrant` | `class` | Fields: `role: string, perm: string` |
| `RbacPathRule` | `class` | Fields: `method: AppMethod, path_prefix: string, required_perm: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rbacStrEql` | `(a: string, b: string): bool` | Invokes `rbacStrEql` and returns `bool`. |
| `rbacStartsWith` | `(s: string, prefix: string): bool` | Invokes `rbacStartsWith` and returns `bool`. |
| `rbacPermMatches` | `(granted: string, needed: string): bool` | Invokes `rbacPermMatches` and returns `bool`. |
| `rbacReset` | `()` | Executes `rbacReset` in-place. |
| `rbacGrant` | `(role: string, perm: string): bool` | Invokes `rbacGrant` and returns `bool`. |
| `rbacProtect` | `(method: AppMethod, path_prefix: string, required_perm: string): bool` | Invokes `rbacProtect` and returns `bool`. |
| `rbacCan` | `(role: string, needed_perm: string): bool` | Invokes `rbacCan` and returns `bool`. |
| `rbacGuardHook` | `(ctx: &mut Context): bool` | Invokes `rbacGuardHook` and returns `bool`. |
| `createRbacPlugin` | `(cfg: RbacConfig): Plugin` | Constructs/initializes an instance via `createRbacPlugin`. |
| `registerRbac` | `(app: &mut App, cfg: RbacConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { rbacStrEql } from "@web/rbac";

pub fn main() {
    let result = rbacStrEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
