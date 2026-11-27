[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/db

Official unified database context and request-scoped automatic transaction plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/db` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/db
```

```cp
import {
    DbDialect,
    DbPluginConfig,
    DbTxStats,
    dbDefaultTxAction,
    dbRunTxAction,
    dbStrEql,
    dbIsWriteMethod,
    dbTxStats,
} from "@web/db";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `DbDialect` | `Sqlite, Postgres, Mysql` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DbPluginConfig` | `class` | Fields: `dialect: DbDialect, dsn: string, auto_tx: bool, tx_header: string` |
| `DbTxStats` | `class` | Fields: `begins: u64, commits: u64, rollbacks: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `dbDefaultTxAction` | `(action: u8): bool` | Invokes `dbDefaultTxAction` and returns `bool`. |
| `dbRunTxAction` | `(action: u8): bool` | Invokes `dbRunTxAction` and returns `bool`. |
| `dbStrEql` | `(a: string, b: string): bool` | Invokes `dbStrEql` and returns `bool`. |
| `dbIsWriteMethod` | `(m: AppMethod): bool` | Invokes `dbIsWriteMethod` and returns `bool`. |
| `dbTxStats` | `(): DbTxStats` | Invokes `dbTxStats` and returns `DbTxStats`. |
| `dbResetStats` | `()` | Executes `dbResetStats` in-place. |
| `dbBeforeHandle` | `(ctx: &mut Context): bool` | Invokes `dbBeforeHandle` and returns `bool`. |
| `dbAfterHandle` | `(ctx: &mut Context, resp: &mut Response)` | Executes `dbAfterHandle` in-place. |
| `createDbPlugin` | `(cfg: DbPluginConfig, action_fn: DbTxActionFn): Plugin` | Constructs/initializes an instance via `createDbPlugin`. |
| `registerDbWithHook` | `(app: &mut App, cfg: DbPluginConfig, action_fn: DbTxActionFn): &mut App` | Registers plugin/middleware into the target application. |
| `registerDb` | `(app: &mut App, cfg: DbPluginConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { dbStrEql } from "@web/db";

pub fn main() {
    let result = dbStrEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
