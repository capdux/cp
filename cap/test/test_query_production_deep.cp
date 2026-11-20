// Source: cp/cap/test/test_query_production_deep.cp
// Deep Production Verification Suite for cap:query & cap.query
// Covers:
//   1. SelectBuilder PostgreSQL $1..$N parameter numbering + DISTINCT/JOIN/WHERE/IN/IS NOT NULL/GROUP BY/ORDER BY/LIMIT/OFFSET
//   2. MySQL `ident` & SQLite ? dialect switching + IdentCodec.quote dot-segments & quote escaping
//   3. InsertBuilder multi-row batch VALUES (...), (...) + RETURNING
//   4. UpdateBuilder / DeleteBuilder & injection isolation (values only in params; invalid identifiers/ops rejected)
//   5. `import cap from "cap"; cap.query` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    QueryError,
    QueryDialect,
    QueryParam,
    BuiltQuery,
    IdentCodec,
    SelectBuilder,
    InsertBuilder,
    UpdateBuilder,
    DeleteBuilder,
} from "cap:query";

fn testSelectPostgresComplex(): !void {
    const alloc = std.heap.page_allocator;

    let sb = SelectBuilder.init(alloc, .Postgres, "public.users");
    defer sb.deinit();

    const cols: [4]string = ["users.id", "users.name", "orgs.title", "users.score"];
    const roles: [3]QueryParam = [.Text("admin"), .Text("editor"), .Text("owner")];

    let q = sb.columns(&cols)
        .distinct()
        .join("orgs", "users.org_id", "orgs.id")
        .leftJoin("profiles", "users.id", "profiles.user_id")
        .whereEq("users.active", .Bool(true))
        .whereOp("users.score", ">=", .Int(80))
        .whereIn("users.role", &roles)
        .whereNotNull("users.verified_at")
        .whereNull("users.deleted_at")
        .orWhereEq("users.vip", .Bool(true))
        .groupBy("users.id")
        .groupBy("orgs.title")
        .orderByDesc("users.score")
        .orderByAsc("users.id")
        .limit(25)
        .offset(50)
        .build()?;
    defer q.deinit();

    const expected_sql =
        "SELECT DISTINCT \"users\".\"id\", \"users\".\"name\", \"orgs\".\"title\", \"users\".\"score\" " ++
        "FROM \"public\".\"users\" " ++
        "INNER JOIN \"orgs\" ON \"users\".\"org_id\" = \"orgs\".\"id\" " ++
        "LEFT JOIN \"profiles\" ON \"users\".\"id\" = \"profiles\".\"user_id\" " ++
        "WHERE \"users\".\"active\" = $1 AND \"users\".\"score\" >= $2 AND \"users\".\"role\" IN ($3, $4, $5) " ++
        "AND \"users\".\"verified_at\" IS NOT NULL AND \"users\".\"deleted_at\" IS NULL OR \"users\".\"vip\" = $6 " ++
        "GROUP BY \"users\".\"id\", \"orgs\".\"title\" " ++
        "ORDER BY \"users\".\"score\" DESC, \"users\".\"id\" ASC " ++
        "LIMIT 25 OFFSET 50";

    expect_eq(std.mem.eql(u8, q.sql, expected_sql), true);
    expect_eq(q.params.len, 6 as usize);
    expect_eq(q.params[0].eql(.Bool(true)), true);
    expect_eq(q.params[1].eql(.Int(80)), true);
    expect_eq(q.params[2].eql(.Text("admin")), true);
    expect_eq(q.params[3].eql(.Text("editor")), true);
    expect_eq(q.params[4].eql(.Text("owner")), true);
    expect_eq(q.params[5].eql(.Bool(true)), true);

    // Default wildcard SELECT *
    let sb_all = SelectBuilder.init(alloc, .Postgres, "audit_logs");
    defer sb_all.deinit();
    let q_all = sb_all.whereEq("level", .Text("warn")).build()?;
    defer q_all.deinit();
    expect_eq(std.mem.eql(u8, q_all.sql, "SELECT * FROM \"audit_logs\" WHERE \"level\" = $1"), true);
    expect_eq(q_all.params.len, 1 as usize);
}

fn testMysqlAndSqliteDialects(): !void {
    const alloc = std.heap.page_allocator;

    // 1. IdentCodec.quote across all 3 dialects + embedded quote escaping
    const pg_q = IdentCodec.quote(alloc, .Postgres, "my_schema.my\"table")?;
    defer alloc.free(pg_q);
    expect_eq(std.mem.eql(u8, pg_q, "\"my_schema\".\"my\"\"table\""), true);

    const my_q = IdentCodec.quote(alloc, .MySql, "db.col`name")?;
    defer alloc.free(my_q);
    expect_eq(std.mem.eql(u8, my_q, "`db`.`col``name`"), true);

    const sq_q = IdentCodec.quote(alloc, .Sqlite, "items.price")?;
    defer alloc.free(sq_q);
    expect_eq(std.mem.eql(u8, sq_q, "\"items\".\"price\""), true);

    // 2. MySQL SelectBuilder with `?` placeholders and backtick identifiers
    let my_sb = SelectBuilder.init(alloc, .MySql, "orders");
    defer my_sb.deinit();
    const my_cols: [2]string = ["orders.id", "orders.total"];
    const my_states: [2]QueryParam = [.Text("paid"), .Text("shipped")];
    let my_res = my_sb.columns(&my_cols)
        .whereOp("orders.total", ">", .Float(99.5))
        .whereIn("orders.state", &my_states)
        .orderByAsc("orders.id")
        .limit(10)
        .build()?;
    defer my_res.deinit();

    expect_eq(
        std.mem.eql(
            u8,
            my_res.sql,
            "SELECT `orders`.`id`, `orders`.`total` FROM `orders` WHERE `orders`.`total` > ? AND `orders`.`state` IN (?, ?) ORDER BY `orders`.`id` ASC LIMIT 10",
        ),
        true,
    );
    expect_eq(my_res.params.len, 3 as usize);
    expect_eq(my_res.params[0].asFloat().? == 99.5, true);
    expect_eq(std.mem.eql(u8, my_res.params[1].asText().?, "paid"), true);
    expect_eq(std.mem.eql(u8, my_res.params[2].asText().?, "shipped"), true);

    // 3. SQLite SelectBuilder with `?` placeholders and double-quote identifiers
    let sq_sb = SelectBuilder.init(alloc, .Sqlite, "sessions");
    defer sq_sb.deinit();
    let sq_res = sq_sb.whereEq("user_id", .Int(7))
        .whereOp("token_hash", "LIKE", .Text("abc%"))
        .limit(1)
        .build()?;
    defer sq_res.deinit();

    expect_eq(
        std.mem.eql(
            u8,
            sq_res.sql,
            "SELECT * FROM \"sessions\" WHERE \"user_id\" = ? AND \"token_hash\" LIKE ? LIMIT 1",
        ),
        true,
    );
    expect_eq(sq_res.params.len, 2 as usize);
    expect_eq(sq_res.params[0].asInt().?, 7);
    expect_eq(std.mem.eql(u8, sq_res.params[1].asText().?, "abc%"), true);
}

fn testInsertBatchAndReturning(): !void {
    const alloc = std.heap.page_allocator;

    // 1. PostgreSQL multi-row batch INSERT + RETURNING
    let ib = InsertBuilder.init(alloc, .Postgres, "metrics");
    defer ib.deinit();

    const cols: [3]string = ["host", "cpu", "blob"];
    const row1: [3]QueryParam = [.Text("node-1"), .Float(0.42), .Bytes("raw\x00A")];
    const row2: [3]QueryParam = [.Text("node-2"), .Null, .Bytes("raw\x00B")];
    const ret: [2]string = ["id", "created_at"];

    let q = ib.columns(&cols)
        .addRow(&row1)
        .addRow(&row2)
        .returning(&ret)
        .build()?;
    defer q.deinit();

    expect_eq(
        std.mem.eql(
            u8,
            q.sql,
            "INSERT INTO \"metrics\" (\"host\", \"cpu\", \"blob\") VALUES ($1, $2, $3), ($4, $5, $6) RETURNING \"id\", \"created_at\"",
        ),
        true,
    );
    expect_eq(q.params.len, 6 as usize);
    expect_eq(std.mem.eql(u8, q.params[0].asText().?, "node-1"), true);
    expect_eq(q.params[1].asFloat().? == 0.42, true);
    expect_eq(std.mem.eql(u8, q.params[2].asBytes().?, "raw\x00A"), true);
    expect_eq(std.mem.eql(u8, q.params[3].asText().?, "node-2"), true);
    expect_eq(q.params[4].isNull(), true);
    expect_eq(std.mem.eql(u8, q.params[5].asBytes().?, "raw\x00B"), true);

    // 2. MySQL multi-row batch INSERT
    let my_ib = InsertBuilder.init(alloc, .MySql, "tags");
    defer my_ib.deinit();
    const t_cols: [1]string = ["name"];
    const t_r1: [1]QueryParam = [.Text("fast")];
    const t_r2: [1]QueryParam = [.Text("safe")];
    let my_q = my_ib.columns(&t_cols).addRow(&t_r1).addRow(&t_r2).build()?;
    defer my_q.deinit();
    expect_eq(std.mem.eql(u8, my_q.sql, "INSERT INTO `tags` (`name`) VALUES (?), (?)"), true);
    expect_eq(my_q.params.len, 2 as usize);

    // 3. EmptyValues error when no rows added or column count mismatch
    let bad_empty = InsertBuilder.init(alloc, .Postgres, "tags");
    defer bad_empty.deinit();
    let hit_empty = false;
    * = bad_empty.build() catch |e| {
        if (e == QueryError.EmptyValues) hit_empty = true;
    };
    expect_eq(hit_empty, true);

    let bad_mismatch = InsertBuilder.init(alloc, .Postgres, "tags");
    defer bad_mismatch.deinit();
    const two_cols: [2]string = ["a", "b"];
    const one_val: [1]QueryParam = [.Int(1)];
    * = bad_mismatch.columns(&two_cols).addRow(&one_val);
    let hit_mismatch = false;
    * = bad_mismatch.build() catch |e| {
        if (e == QueryError.EmptyValues) hit_mismatch = true;
    };
    expect_eq(hit_mismatch, true);
}

fn testUpdateDeleteAndInjectionIsolation(): !void {
    const alloc = std.heap.page_allocator;

    // 1. UpdateBuilder with hostile SQL payloads in parameter values (must NEVER enter SQL string)
    let payload_buf: [32]u8 = undefined;
    const hostile_val = "'; DROP TABLE users; --";
    @memcpy(payload_buf[0..hostile_val.len], hostile_val);

    let ub = UpdateBuilder.init(alloc, .Postgres, "users");
    defer ub.deinit();

    const ret_u: [1]string = ["updated_at"];
    let uq = ub.set("bio", .Text(payload_buf[0..hostile_val.len]))
        .set("retries", .Int(3))
        .whereEq("username", .Text("admin\" OR \"1\"=\"1"))
        .whereOp("id", ">", .Int(0))
        .returning(&ret_u)
        .build()?;
    defer uq.deinit();

    // Overwrite caller buffer to verify ownership-independent parameter cloning
    @memcpy(payload_buf[0..hostile_val.len], "XXXXXXXXXXXXXXXXXXXXXXX");

    expect_eq(
        uq.sql,
        "UPDATE \"users\" SET \"bio\" = $1, \"retries\" = $2 WHERE \"username\" = $3 AND \"id\" > $4 RETURNING \"updated_at\"",
    );
    expect_eq(if (std.mem.indexOf(u8, uq.sql, "DROP TABLE") == null) (1 as i32) else (0 as i32), 1 as i32);
    expect_eq(if (std.mem.indexOf(u8, uq.sql, "OR") == null) (2 as i32) else (0 as i32), 2 as i32);
    expect_eq(uq.params.len, 4 as usize);
    expect_eq(uq.params[0].asText().?, hostile_val);
    expect_eq(uq.params[1].asInt().?, 3);
    expect_eq(uq.params[2].asText().?, "admin\" OR \"1\"=\"1");
    expect_eq(uq.params[3].asInt().?, 0);

    // 2. DeleteBuilder with WHERE IN + RETURNING
    let db = DeleteBuilder.init(alloc, .Postgres, "sessions");
    defer db.deinit();
    const del_ids: [2]QueryParam = [.Int(101), .Int(102)];
    const del_ret: [1]string = ["id"];
    let dq = db.whereIn("id", &del_ids)
        .whereNull("revoked_at")
        .returning(&del_ret)
        .build()?;
    defer dq.deinit();

    expect_eq(
        dq.sql,
        "DELETE FROM \"sessions\" WHERE \"id\" IN ($1, $2) AND \"revoked_at\" IS NULL RETURNING \"id\"",
    );
    expect_eq(dq.params.len, 2 as usize);
    expect_eq(dq.params[0].asInt().?, 101);
    expect_eq(dq.params[1].asInt().?, 102);

    // 3. Reject malformed identifiers: NUL byte, semicolon, SQL comments, empty segments
    const bad_idents: [7]string = [
        "",
        "users\x00admin",
        "users; DROP TABLE x",
        "users--comment",
        "users/*comment*/",
        ".leading_dot",
        "trailing_dot.",
    ];
    let idx: usize = 0;
    while (idx < bad_idents.len) : (idx += 1) {
        const ident = bad_idents[idx];
        let rejected = false;
        * = IdentCodec.quote(alloc, .Postgres, ident) catch |e| {
            if (e == QueryError.InvalidIdentifier) rejected = true;
        };
        expect_eq(if (rejected) idx else (999 as usize), idx);
    }

    // 4. Reject empty table and invalid whereOp operator
    let empty_tbl_sb = SelectBuilder.init(alloc, .Postgres, "");
    defer empty_tbl_sb.deinit();
    let hit_empty_tbl = false;
    * = empty_tbl_sb.build() catch |e| {
        if (e == QueryError.EmptyTable) hit_empty_tbl = true;
    };
    expect_eq(if (hit_empty_tbl) (100 as i32) else (0 as i32), 100 as i32);

    let bad_op_sb = SelectBuilder.init(alloc, .Postgres, "users");
    defer bad_op_sb.deinit();
    * = bad_op_sb.whereOp("id", "= 1; DROP TABLE users;--", .Int(1));
    let hit_bad_op = false;
    * = bad_op_sb.build() catch |e| {
        if (e == QueryError.InvalidIdentifier) hit_bad_op = true;
    };
    expect_eq(hit_bad_op, true);
}

fn testCapQueryDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.query.*` automatic dynamic mounting
    const quoted = cap.query.IdentCodec.quote(alloc, cap.query.QueryDialect.MySql, "app.users")?;
    defer alloc.free(quoted);
    expect_eq(std.mem.eql(u8, quoted, "`app`.`users`"), true);

    let sb = cap.query.SelectBuilder.init(alloc, .Sqlite, "tasks");
    defer sb.deinit();
    const cols: [2]string = ["id", "title"];
    let sq = sb.columns(&cols)
        .whereEq("done", cap.query.QueryParam.makeBool(false))
        .limit(5)
        .build()?;
    defer sq.deinit();
    expect_eq(std.mem.eql(u8, sq.sql, "SELECT \"id\", \"title\" FROM \"tasks\" WHERE \"done\" = ? LIMIT 5"), true);
    expect_eq(sq.params.len, 1 as usize);

    let ib = cap.query.InsertBuilder.init(alloc, .Postgres, "tasks");
    defer ib.deinit();
    const icols: [1]string = ["title"];
    const irow: [1]cap.query.QueryParam = [.Text("ship cap:query")];
    const iret: [1]string = ["id"];
    let iq = ib.columns(&icols).addRow(&irow).returning(&iret).build()?;
    defer iq.deinit();
    expect_eq(std.mem.eql(u8, iq.sql, "INSERT INTO \"tasks\" (\"title\") VALUES ($1) RETURNING \"id\""), true);

    let ub = cap.query.UpdateBuilder.init(alloc, .Postgres, "tasks");
    defer ub.deinit();
    let uq = ub.set("done", .Bool(true)).whereEq("id", .Int(1)).build()?;
    defer uq.deinit();
    expect_eq(std.mem.eql(u8, uq.sql, "UPDATE \"tasks\" SET \"done\" = $1 WHERE \"id\" = $2"), true);

    let db = cap.query.DeleteBuilder.init(alloc, .Sqlite, "tasks");
    defer db.deinit();
    let dq = db.whereEq("done", .Bool(true)).build()?;
    defer dq.deinit();
    expect_eq(std.mem.eql(u8, dq.sql, "DELETE FROM \"tasks\" WHERE \"done\" = ?"), true);
}

fn main() {
    test("1. SelectBuilder PostgreSQL $1..$N + JOIN/WHERE/IN/IS NOT NULL/ORDER BY/LIMIT/OFFSET", () => {
        testSelectPostgresComplex()?;
    });
    test("2. MySQL `ident` & SQLite ? Dialect Switching + IdentCodec Quote Escaping", () => {
        testMysqlAndSqliteDialects()?;
    });
    test("3. InsertBuilder Multi-Row Batch VALUES + RETURNING & Boundary Guards", () => {
        testInsertBatchAndReturning()?;
    });
    test("4. UpdateBuilder/DeleteBuilder & Hostile Payload/Identifier Injection Isolation", () => {
        testUpdateDeleteAndInjectionIsolation()?;
    });
    test("5. import cap from \"cap\"; cap.query Automatic Dynamic Mounting", () => {
        testCapQueryDynamicMount()?;
    });
}
