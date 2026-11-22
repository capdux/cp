// Source: cp/cap/test/test_aof_production_deep.cp (Production deep tests for cap:aof)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AofArgIter,
    AofCommand,
    AofIter,
    AofStats,
    isAofMixedRdb,
    isAofResp,
    summarizeAof,
} from "cap:binfmt";

test("1. multi-command SELECT / SET / HSET AOF stream iteration and arg extraction", () => {
    const aof =
        "*2\r\n$6\r\nSELECT\r\n$1\r\n0\r\n*3\r\n$3\r\nSET\r\n$4\r\nuser\r\n$5\r\nalice\r\n*4\r\n$4\r\nHSET\r\n$4\r\nmeta\r\n$4\r\nrole\r\n$5\r\nadmin\r\n";

    expect_eq(isAofResp(aof), true);
    expect_eq(isAofMixedRdb(aof), false);

    let it = AofIter.init(aof);

    const c0 = it.next() ?? unreachable;
    expect_eq(c0.arg_count, 2);
    expect_eq(std.mem.eql(u8, c0.name() ?? "", "SELECT"), true);
    expect_eq(std.mem.eql(u8, c0.argAt(1) ?? "", "0"), true);
    expect_eq(c0.argAt(2) == null, true);

    const c1 = it.next() ?? unreachable;
    expect_eq(c1.arg_count, 3);
    expect_eq(std.mem.eql(u8, c1.name() ?? "", "SET"), true);
    expect_eq(std.mem.eql(u8, c1.argAt(1) ?? "", "user"), true);
    expect_eq(std.mem.eql(u8, c1.argAt(2) ?? "", "alice"), true);

    const c2 = it.next() ?? unreachable;
    expect_eq(c2.arg_count, 4);
    expect_eq(std.mem.eql(u8, c2.name() ?? "", "HSET"), true);
    expect_eq(std.mem.eql(u8, c2.argAt(3) ?? "", "admin"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
    expect_eq(it.consumedBytes(), aof.len);

    const stats = summarizeAof(aof);
    expect_eq(stats.command_count, 3);
    expect_eq(stats.arg_count, 9);
    expect_eq(stats.consumed_bytes, aof.len);
    expect_eq(stats.truncated_or_invalid, false);
});

test("2. binary-safe Bulk Strings containing embedded CRLF and NUL bytes", () => {
    const bin_aof = "*3\r\n$3\r\nSET\r\n$3\r\nbin\r\n$8\r\na\r\n\x00b\r\nc\r\n";
    let it = AofIter.init(bin_aof);
    const cmd = it.next() ?? unreachable;
    expect_eq(cmd.arg_count, 3);
    const val = cmd.argAt(2) ?? unreachable;
    expect_eq(val.len, 8);
    expect_eq(val[0], 'a');
    expect_eq(val[1], '\r');
    expect_eq(val[2], '\n');
    expect_eq(val[3], 0);
    expect_eq(val[4], 'b');
    expect_eq(val[7], 'c');
    expect_eq(it.had_error, false);
});

test("3. Redis 7.0 #TS: timestamp annotations bind to the immediately following command", () => {
    const ts_aof =
        "#TS:1700000123\r\n*2\r\n$4\r\nPING\r\n$2\r\nhi\r\n# generic comment\r\n*2\r\n$3\r\nDEL\r\n$1\r\nk\r\n";

    expect_eq(isAofResp(ts_aof), true);

    let it = AofIter.init(ts_aof);
    const c0 = it.next() ?? unreachable;
    expect_eq(c0.timestamp ?? 0, 1700000123);
    expect_eq(std.mem.eql(u8, c0.name() ?? "", "PING"), true);

    // Second command has no #TS: annotation => timestamp is null
    const c1 = it.next() ?? unreachable;
    expect_eq(c1.timestamp == null, true);
    expect_eq(std.mem.eql(u8, c1.name() ?? "", "DEL"), true);
    expect_eq(it.had_error, false);
});

test("4. mixed RDB+AOF preamble detection distinguishes REDIS header from pure RESP", () => {
    const mixed = "REDIS0011\xFA\x09redis-ver\x057.2.0";
    expect_eq(isAofMixedRdb(mixed), true);
    expect_eq(isAofResp(mixed), false);

    const short_rdb = "REDI";
    expect_eq(isAofMixedRdb(short_rdb), false);
    expect_eq(isAofResp(""), false);
});

test("5. truncated tail command preserves safe consumedBytes and rejects malformed lengths", () => {
    const valid_prefix = "*2\r\n$4\r\nPING\r\n$2\r\nok\r\n";
    const with_trunc_tail = "*2\r\n$4\r\nPING\r\n$2\r\nok\r\n*3\r\n$3\r\nSET\r\n$5\r\nhe";

    const stats = summarizeAof(with_trunc_tail);
    expect_eq(stats.command_count, 1);
    expect_eq(stats.arg_count, 2);
    expect_eq(stats.truncated_or_invalid, true);
    // consumed_bytes stops right at the end of the last valid command!
    expect_eq(stats.consumed_bytes, valid_prefix.len);

    // Malformed negative length or missing CRLF
    const bad_neg = "*1\r\n$-1\r\n";
    const s_neg = summarizeAof(bad_neg);
    expect_eq(s_neg.command_count, 0);
    expect_eq(s_neg.truncated_or_invalid, true);
    expect_eq(s_neg.consumed_bytes, 0);

    const bad_crlf = "*1\r\n$3\r\nabcXX";
    const s_crlf = summarizeAof(bad_crlf);
    expect_eq(s_crlf.command_count, 0);
    expect_eq(s_crlf.truncated_or_invalid, true);
});
