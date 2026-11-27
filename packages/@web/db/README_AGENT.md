# @web/db@1.0.0 (Agent Quickref)

> Official unified database context and request-scoped automatic transaction plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/db";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum DbDialect`

## Functions
- `export fn dbDefaultTxAction(action: u8): bool;`
- `export fn dbRunTxAction(action: u8): bool;`
- `export fn dbStrEql(a: string, b: string): bool;`
- `export fn dbIsWriteMethod(m: AppMethod): bool;`
- `export fn dbTxStats(): DbTxStats;`
- `export fn dbResetStats();`
- `export fn dbBeforeHandle(ctx: &mut Context): bool;`
- `export fn dbAfterHandle(ctx: &mut Context, resp: &mut Response);`
- `export fn createDbPlugin(cfg: DbPluginConfig, action_fn: DbTxActionFn): Plugin;`
- `export fn registerDbWithHook(app: &mut App, cfg: DbPluginConfig, action_fn: DbTxActionFn): &mut App;`
- `export fn registerDb(app: &mut App, cfg: DbPluginConfig): &mut App;`
