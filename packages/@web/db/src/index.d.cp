// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export enum DbDialect {
    Sqlite,
    Postgres,
    Mysql
}

export class DbPluginConfig {
    dialect: DbDialect,
    dsn: string,
    auto_tx: bool,
    tx_header: string,
}

export class DbTxStats {
    begins: u64,
    commits: u64,
    rollbacks: u64,
}

export fn dbDefaultTxAction(action: u8): bool;
export fn dbRunTxAction(action: u8): bool;
export fn dbStrEql(a: string, b: string): bool;
export fn dbIsWriteMethod(m: AppMethod): bool;
export fn dbTxStats(): DbTxStats;
export fn dbResetStats();
export fn dbBeforeHandle(ctx: &mut Context): bool;
export fn dbAfterHandle(ctx: &mut Context, resp: &mut Response);
export fn createDbPlugin(cfg: DbPluginConfig, action_fn: DbTxActionFn): Plugin;
export fn registerDbWithHook(app: &mut App, cfg: DbPluginConfig, action_fn: DbTxActionFn): &mut App;
export fn registerDb(app: &mut App, cfg: DbPluginConfig): &mut App;
