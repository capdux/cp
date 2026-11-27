# @web/db — AI Agent Quick Ref

Official unified database context and request-scoped automatic transaction plugin for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum DbDialect { Sqlite, Postgres, Mysql }
class DbPluginConfig { dialect: DbDialect, dsn: string, auto_tx: bool, tx_header: string }
class DbTxStats { begins: u64, commits: u64, rollbacks: u64 }
```

## API Signatures

```cp
fn dbDefaultTxAction(action: u8): bool;
fn dbRunTxAction(action: u8): bool;
fn dbStrEql(a: string, b: string): bool;
fn dbIsWriteMethod(m: AppMethod): bool;
fn dbTxStats(): DbTxStats;
fn dbResetStats();
fn dbBeforeHandle(ctx: &mut Context): bool;
fn dbAfterHandle(ctx: &mut Context, resp: &mut Response);
fn createDbPlugin(cfg: DbPluginConfig, action_fn: DbTxActionFn): Plugin;
fn registerDbWithHook(app: &mut App, cfg: DbPluginConfig, action_fn: DbTxActionFn): &mut App;
fn registerDb(app: &mut App, cfg: DbPluginConfig): &mut App;
```

## Minimal Usage

```cp
import { dbStrEql } from "@web/db";

pub fn main() {
    let result = dbStrEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/db"`; install via `cap add @web/db`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
