// Production Deep Tests for cap:pgwire CommandTag (lib/pgwire/command_tag.cp)
import { test, expect_eq } from "cap:test";
import {
    PgCommandTagError,
    PgCommandKind,
    PgCommandTag,
} from "cap:pgwire";

test("pgwire command_tag: INSERT <oid> <rows> parsing and formatting", fn() {
    const t1 = PgCommandTag.parse("INSERT 0 1");
    expect_eq(t1.kind, PgCommandKind.Insert);
    expect_eq(t1.oid, @as(u64, 0));
    expect_eq(t1.rows, @as(u64, 1));
    expect_eq(t1.rowCount() ?? 999, @as(u64, 1));
    expect_eq(t1.commandName(), "INSERT");
    expect_eq(t1.isDml(), true);
    expect_eq(t1.isQueryOrCursor(), false);

    const t2 = PgCommandTag.parse("INSERT 16384 250000");
    expect_eq(t2.kind, PgCommandKind.Insert);
    expect_eq(t2.oid, @as(u64, 16384));
    expect_eq(t2.rows, @as(u64, 250000));

    let buf: [64]u8 = [0; 64];
    const n1 = t1.format(buf[0..]) catch unreachable;
    expect_eq(buf[0..n1], "INSERT 0 1");

    const n2 = t2.format(buf[0..]) catch unreachable;
    expect_eq(buf[0..n2], "INSERT 16384 250000");
});

test("pgwire command_tag: all 7 single-count commands (DELETE/UPDATE/MERGE/SELECT/MOVE/FETCH/COPY)", fn() {
    const t_del = PgCommandTag.parse("DELETE 7");
    expect_eq(t_del.kind, PgCommandKind.Delete);
    expect_eq(t_del.rows, @as(u64, 7));
    expect_eq(t_del.isDml(), true);
    expect_eq(t_del.isQueryOrCursor(), false);

    const t_upd = PgCommandTag.parse("UPDATE 0");
    expect_eq(t_upd.kind, PgCommandKind.Update);
    expect_eq(t_upd.rows, @as(u64, 0));
    expect_eq(t_upd.isDml(), true);

    const t_mrg = PgCommandTag.parse("MERGE 42");
    expect_eq(t_mrg.kind, PgCommandKind.Merge);
    expect_eq(t_mrg.rows, @as(u64, 42));
    expect_eq(t_mrg.isDml(), true);

    const t_sel = PgCommandTag.parse("SELECT 1024");
    expect_eq(t_sel.kind, PgCommandKind.Select);
    expect_eq(t_sel.rows, @as(u64, 1024));
    expect_eq(t_sel.isDml(), false);
    expect_eq(t_sel.isQueryOrCursor(), true);

    const t_mov = PgCommandTag.parse("MOVE 15");
    expect_eq(t_mov.kind, PgCommandKind.Move);
    expect_eq(t_mov.rows, @as(u64, 15));
    expect_eq(t_mov.isQueryOrCursor(), true);

    const t_fet = PgCommandTag.parse("FETCH 50");
    expect_eq(t_fet.kind, PgCommandKind.Fetch);
    expect_eq(t_fet.rows, @as(u64, 50));
    expect_eq(t_fet.isQueryOrCursor(), true);

    const t_cpy = PgCommandTag.parse("COPY 999999");
    expect_eq(t_cpy.kind, PgCommandKind.Copy);
    expect_eq(t_cpy.rows, @as(u64, 999999));
    expect_eq(t_cpy.isDml(), false);
    expect_eq(t_cpy.isQueryOrCursor(), false);

    let buf: [64]u8 = [0; 64];
    const n_sel = t_sel.format(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_sel], "SELECT 1024");
    const n_cpy = t_cpy.format(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_cpy], "COPY 999999");
});

test("pgwire command_tag: DDL, transaction, and unknown tags fall back safely to Other", fn() {
    const t_ddl = PgCommandTag.parse("CREATE TABLE");
    expect_eq(t_ddl.kind, PgCommandKind.Other);
    expect_eq(t_ddl.rowCount() == null, true);
    expect_eq(t_ddl.commandName(), "CREATE TABLE");
    expect_eq(t_ddl.isDml(), false);
    expect_eq(t_ddl.isQueryOrCursor(), false);

    const t_begin = PgCommandTag.parse("BEGIN");
    expect_eq(t_begin.kind, PgCommandKind.Other);
    expect_eq(t_begin.raw, "BEGIN");

    const t_empty = PgCommandTag.parse("");
    expect_eq(t_empty.kind, PgCommandKind.Other);
    expect_eq(t_empty.raw, "");

    let buf: [64]u8 = [0; 64];
    const n_ddl = t_ddl.format(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_ddl], "CREATE TABLE");
});

test("pgwire command_tag: u64 boundary and overflow fallback protection", fn() {
    // Exact u64::MAX = 18446744073709551615
    const t_max = PgCommandTag.parse("SELECT 18446744073709551615");
    expect_eq(t_max.kind, PgCommandKind.Select);
    expect_eq(t_max.rows, @as(u64, 18446744073709551615));

    let buf: [64]u8 = [0; 64];
    const n_max = t_max.format(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_max], "SELECT 18446744073709551615");

    // u64::MAX + 1 -> safe fallback to Other
    const t_ovf = PgCommandTag.parse("SELECT 18446744073709551616");
    expect_eq(t_ovf.kind, PgCommandKind.Other);
    expect_eq(t_ovf.rowCount() == null, true);

    // Extra digit overflow
    const t_ovf2 = PgCommandTag.parse("INSERT 0 99999999999999999999");
    expect_eq(t_ovf2.kind, PgCommandKind.Other);
});

test("pgwire command_tag: malformed inputs, case sensitivity, equality, and BufferTooSmall", fn() {
    // PostgreSQL protocol commands are strictly uppercase
    expect_eq(PgCommandTag.parse("select 10").kind, PgCommandKind.Other);
    // Missing second field on INSERT
    expect_eq(PgCommandTag.parse("INSERT 5").kind, PgCommandKind.Other);
    // Non-numeric count or trailing whitespace
    expect_eq(PgCommandTag.parse("DELETE abc").kind, PgCommandKind.Other);
    expect_eq(PgCommandTag.parse("UPDATE 12 ").kind, PgCommandKind.Other);
    expect_eq(PgCommandTag.parse("UPDATE -5").kind, PgCommandKind.Other);
    expect_eq(PgCommandTag.parse("FETCH ").kind, PgCommandKind.Other);
    expect_eq(PgCommandTag.parse("INSERT 0 ").kind, PgCommandKind.Other);
    expect_eq(PgCommandTag.parse("INSERT bad 3").kind, PgCommandKind.Other);

    // Equality checks
    expect_eq(PgCommandTag.parse("UPDATE 9").eql(PgCommandTag.parse("UPDATE 9")), true);
    expect_eq(PgCommandTag.parse("UPDATE 9").eql(PgCommandTag.parse("UPDATE 10")), false);
    expect_eq(PgCommandTag.parse("INSERT 0 3").eql(PgCommandTag.parse("INSERT 1 3")), false);
    expect_eq(PgCommandTag.parse("COMMIT").eql(PgCommandTag.parse("COMMIT")), true);
    expect_eq(PgCommandTag.parse("COMMIT").eql(PgCommandTag.parse("ROLLBACK")), false);

    // BufferTooSmall protection
    const tag = PgCommandTag.parse("INSERT 0 12345");
    let tiny: [6]u8 = [0; 6];
    let got_err = false;
    * = tag.format(tiny[0..]) catch |e| blk: {
        expect_eq(e, PgCommandTagError.BufferTooSmall);
        got_err = true;
        break :blk 0;
    };
    expect_eq(got_err, true);
});
