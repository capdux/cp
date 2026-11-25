// Origin: Official @web/db Plugin (Pure-CP Unified DB Context & Request-Scoped Auto-Tx) | Created: 2026-11-25
// ============================================================================
// @web/db — Unified Database Context & Automatic Request-Scoped Transactions
//   - Inspired by Fastify `@fastify/postgres` & Axum `axum-sqlx-tx`
//   - Dialect-agnostic configuration (`Sqlite` / `Postgres` / `Mysql`)
//   - Automatic `BEGIN` on write requests (`POST`/`PUT`/`PATCH`/`DELETE`),
//     auto `COMMIT` on `status < 400`, auto `ROLLBACK` on `status >= 400`
//   - Pluggable driver lifecycle hook (`0=BEGIN`, `1=COMMIT`, `2=ROLLBACK`)
//   - Zero heap allocations, zero external C dependencies
// ============================================================================

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const DB_TX_BEGIN: u8 = 0;
pub const DB_TX_COMMIT: u8 = 1;
pub const DB_TX_ROLLBACK: u8 = 2;

pub enum DbDialect {
    Sqlite,
    Postgres,
    Mysql,

    pub fn asString(): string {
        match (_) {
            .Sqlite => return "sqlite",
            .Postgres => return "postgres",
            .Mysql => return "mysql",
        }
    }
}

pub struct DbPluginConfig {
    dialect: DbDialect = DbDialect.Sqlite;
    dsn: string = ":memory:";
    auto_tx: bool = true;
    tx_header: string = "X-DB-Tx";
}

pub struct DbTxStats {
    begins: u64 = 0;
    commits: u64 = 0;
    rollbacks: u64 = 0;
}

pub type DbTxActionFn = *const fn (u8): bool;

// ============================================================================
// Internal Prefix-Scoped Helpers & State (db*)
// ============================================================================

fn dbDefaultTxAction(action: u8): bool {
    * = action;
    return true;
}

let g_db_cfg: DbPluginConfig = DbPluginConfig{};
let g_db_action_fn: DbTxActionFn? = null;
let g_db_stats: DbTxStats = DbTxStats{};

fn dbRunTxAction(action: u8): bool {
    if (hook := g_db_action_fn) {
        return hook(action);
    }
    return dbDefaultTxAction(action);
}

fn dbStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn dbIsWriteMethod(m: AppMethod): bool {
    match (m) {
        .POST => return true,
        .PUT => return true,
        .PATCH => return true,
        .DELETE => return true,
        else => return false,
    }
}

// ============================================================================
// Public @web/db API
// ============================================================================

/// Read cumulative transaction lifecycle counters (`begins`, `commits`, `rollbacks`).
pub fn dbTxStats(): DbTxStats {
    return g_db_stats;
}

/// Reset transaction lifecycle counters (useful between test cases).
pub fn dbResetStats() {
    g_db_stats = DbTxStats{};
}

/// `onBeforeHandle` lifecycle hook: starts a transaction on non-read-only requests
/// when `auto_tx` is enabled, injecting `ctx.setDerived("db_tx_active", "1")`.
pub fn dbBeforeHandle(ctx: *Context): bool {
    if (!g_db_cfg.auto_tx) return true;
    if (!dbIsWriteMethod(ctx.method)) return true;

    if (!dbRunTxAction(DB_TX_BEGIN)) {
        ctx.abort(Response.json(503, "{\"error\":\"db_tx_begin_failed\"}"));
        return false;
    }
    g_db_stats.begins += 1;
    * = ctx.setDerived("db_tx_active", "1");
    return true;
}

/// `onAfterHandle` lifecycle hook: commits active transactions on `status < 400`
/// and rolls them back on `status >= 400`, attaching `X-DB-Tx` response header.
pub fn dbAfterHandle(ctx: *Context, resp: *Response) {
    if (active := ctx.derived("db_tx_active")) {
        if (!dbStrEql(active, "1")) return;
        const hdr_name = if (g_db_cfg.tx_header.len > 0) g_db_cfg.tx_header else "X-DB-Tx";
        if (resp.status < 400) {
            if (dbRunTxAction(DB_TX_COMMIT)) {
                g_db_stats.commits += 1;
                * = resp.setHeader(hdr_name, "committed");
            } else {
                * = dbRunTxAction(DB_TX_ROLLBACK);
                g_db_stats.rollbacks += 1;
                * = resp.setHeader(hdr_name, "rolled_back");
            }
        } else {
            * = dbRunTxAction(DB_TX_ROLLBACK);
            g_db_stats.rollbacks += 1;
            * = resp.setHeader(hdr_name, "rolled_back");
        }
    }
}

/// Create a composable Elysia-style `Plugin` for `@web/db`.
pub fn createDbPlugin(cfg: DbPluginConfig, action_fn: DbTxActionFn): Plugin {
    g_db_cfg = cfg;
    g_db_action_fn = action_fn;
    let p = Plugin.init("@web/db");
    * = p.withScope(PluginScope.Global);
    * = p.setState("db_dialect", cfg.dialect.asString());
    * = p.setState("db_dsn", cfg.dsn);
    * = p.onBeforeHandle(dbBeforeHandle);
    * = p.onAfterHandle(dbAfterHandle);
    return p;
}

/// Register `@web/db` with a custom driver transaction lifecycle hook (fluent chainable).
pub fn registerDbWithHook(app: *App, cfg: DbPluginConfig, action_fn: DbTxActionFn): *App {
    const p = createDbPlugin(cfg, action_fn);
    return app.usePlugin(&p);
}

/// Register `@web/db` with the default zero-I/O transaction hook (fluent chainable).
pub fn registerDb(app: *App, cfg: DbPluginConfig): *App {
    return registerDbWithHook(app, cfg, dbDefaultTxAction);
}
