// Unit tests for @web/db
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    DbDialect,
    DbPluginConfig,
    DbTxStats,
    DbTxActionFn,
    DB_TX_BEGIN,
    DB_TX_COMMIT,
    DB_TX_ROLLBACK,
    dbBeforeHandle,
    dbAfterHandle,
    dbTxStats,
    dbResetStats,
    registerDb,
    registerDbWithHook,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

let g_test_last_action: u8 = 255;
let g_test_fail_begin: bool = false;

fn customTxHook(action: u8): bool {
    g_test_last_action = action;
    if (action == DB_TX_BEGIN && g_test_fail_begin) {
        return false;
    }
    return true;
}

fn readHandler(ctx: *Context): Response {
    const dialect = ctx.stateVal("db_dialect") orelse "";
    const dsn = ctx.stateVal("db_dsn") orelse "";
    const tx_active = ctx.derived("db_tx_active");
    if (strEql(dialect, "postgres") && strEql(dsn, "postgres://localhost/app") && tx_active == null) {
        return Response.json(200, "{\"read\":true}");
    }
    return Response.json(500, "{\"read\":false}");
}

fn createOrderOkHandler(ctx: *Context): Response {
    const active = ctx.derived("db_tx_active") orelse "";
    if (strEql(active, "1")) {
        return Response.json(201, "{\"created\":true}");
    }
    return Response.json(500, "{\"error\":\"missing_tx\"}");
}

fn createOrderFailHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(422, "{\"error\":\"validation_failed\"}");
}

pub fn main() {
    dbResetStats();

    const cfg = DbPluginConfig{
        dialect: DbDialect.Postgres,
        dsn: "postgres://localhost/app",
        auto_tx: true,
        tx_header: "X-DB-Tx",
    };

    let app = App.init();
    * = registerDbWithHook(*app, cfg, customTxHook);
    * = app.get("/orders", readHandler);
    * = app.post("/orders", createOrderOkHandler);
    * = app.post("/orders/fail", createOrderFailHandler);

    // 1. Read-only GET does not start a transaction
    const r_get = app.dispatch(Request.init(AppMethod.GET, "/orders"));
    assertTrue(r_get.status == 200, "GET /orders returns 200 with db state");
    assertTrue(r_get.header("X-DB-Tx") == null, "GET /orders has no X-DB-Tx header");
    const st0 = dbTxStats();
    assertTrue(st0.begins == 0 && st0.commits == 0 && st0.rollbacks == 0, "GET does not increment tx stats");

    // 2. Write POST returning 201 -> auto BEGIN + auto COMMIT
    const r_ok = app.dispatch(Request.init(AppMethod.POST, "/orders"));
    assertTrue(r_ok.status == 201, "POST /orders returns 201");
    assertTrue(strEql(r_ok.header("X-DB-Tx") orelse "", "committed"), "201 response marked committed");
    assertTrue(g_test_last_action == DB_TX_COMMIT, "hook received COMMIT");
    const st1 = dbTxStats();
    assertTrue(st1.begins == 1 && st1.commits == 1 && st1.rollbacks == 0, "stats recorded 1 begin + 1 commit");

    // 3. Write POST returning 422 -> auto BEGIN + auto ROLLBACK
    const r_fail = app.dispatch(Request.init(AppMethod.POST, "/orders/fail"));
    assertTrue(r_fail.status == 422, "POST /orders/fail returns 422");
    assertTrue(strEql(r_fail.header("X-DB-Tx") orelse "", "rolled_back"), "422 response marked rolled_back");
    assertTrue(g_test_last_action == DB_TX_ROLLBACK, "hook received ROLLBACK");
    const st2 = dbTxStats();
    assertTrue(st2.begins == 2 && st2.commits == 1 && st2.rollbacks == 1, "stats recorded 2 begins + 1 commit + 1 rollback");

    // 4. Hook BEGIN failure aborts with 503
    g_test_fail_begin = true;
    const r_err = app.dispatch(Request.init(AppMethod.POST, "/orders"));
    assertTrue(r_err.status == 503, "BEGIN failure aborts with 503");
    g_test_fail_begin = false;

    // 5. registerDb default hook works out of the box
    dbResetStats();
    let app2 = App.init();
    * = registerDb(*app2, DbPluginConfig{});
    * = app2.post("/items", createOrderOkHandler);
    const r_def = app2.dispatch(Request.init(AppMethod.POST, "/items"));
    assertTrue(r_def.status == 201, "registerDb default hook succeeds");
    assertTrue(strEql(r_def.header("X-DB-Tx") orelse "", "committed"), "default hook commits");
    const st3 = dbTxStats();
    assertTrue(st3.begins == 1 && st3.commits == 1 && st3.rollbacks == 0, "default hook stats accurate");

    std.debug.print("PASS: @web/db all tests passed!\n", .{});
}
