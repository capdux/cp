import std from "std";
import { test, expect_eq } from "cap:test";
import { SqlStateClass, SqlStateCode } from "cap:pg_sqlstate";

test("1. SqlStateCode.class classifies all 15 SQLSTATE classes and prefixes", () => {
    const c00 = SqlStateCode.parse("00000") ?? unreachable;
    expect_eq(c00.class() == SqlStateClass.success, true);
    expect_eq(c00.isSuccess(), true);
    expect_eq(std.mem.eql(u8, c00.class().prefix(), "00"), true);

    expect_eq((SqlStateCode.parse("01000") ?? unreachable).class() == SqlStateClass.warning, true);
    expect_eq((SqlStateCode.parse("02000") ?? unreachable).class() == SqlStateClass.no_data, true);
    expect_eq((SqlStateCode.parse("08006") ?? unreachable).class() == SqlStateClass.connection_exception, true);
    expect_eq((SqlStateCode.parse("22P02") ?? unreachable).class() == SqlStateClass.data_exception, true);
    expect_eq((SqlStateCode.parse("23505") ?? unreachable).class() == SqlStateClass.integrity_constraint_violation, true);
    expect_eq((SqlStateCode.parse("25P02") ?? unreachable).class() == SqlStateClass.invalid_transaction_state, true);
    expect_eq((SqlStateCode.parse("28P01") ?? unreachable).class() == SqlStateClass.invalid_authorization, true);
    expect_eq((SqlStateCode.parse("40001") ?? unreachable).class() == SqlStateClass.transaction_rollback, true);
    expect_eq((SqlStateCode.parse("42601") ?? unreachable).class() == SqlStateClass.syntax_or_access_error, true);
    expect_eq((SqlStateCode.parse("53300") ?? unreachable).class() == SqlStateClass.insufficient_resources, true);
    expect_eq((SqlStateCode.parse("57014") ?? unreachable).class() == SqlStateClass.operator_intervention, true);
    expect_eq((SqlStateCode.parse("58030") ?? unreachable).class() == SqlStateClass.system_error, true);
    expect_eq((SqlStateCode.parse("XX000") ?? unreachable).class() == SqlStateClass.internal_error, true);
    expect_eq((SqlStateCode.parse("99ZZZ") ?? unreachable).class() == SqlStateClass.other, true);
    expect_eq(std.mem.eql(u8, SqlStateClass.other.prefix(), "??"), true);
});

test("2. conditionName covers 25+ high-frequency PostgreSQL SQLSTATE codes", () => {
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("23505") ?? unreachable).conditionName() ?? unreachable, "unique_violation"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("23503") ?? unreachable).conditionName() ?? unreachable, "foreign_key_violation"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("23502") ?? unreachable).conditionName() ?? unreachable, "not_null_violation"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("23514") ?? unreachable).conditionName() ?? unreachable, "check_violation"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("23P01") ?? unreachable).conditionName() ?? unreachable, "exclusion_violation"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("40001") ?? unreachable).conditionName() ?? unreachable, "serialization_failure"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("40P01") ?? unreachable).conditionName() ?? unreachable, "deadlock_detected"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("42601") ?? unreachable).conditionName() ?? unreachable, "syntax_error"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("42501") ?? unreachable).conditionName() ?? unreachable, "insufficient_privilege"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("42P01") ?? unreachable).conditionName() ?? unreachable, "undefined_table"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("42703") ?? unreachable).conditionName() ?? unreachable, "undefined_column"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("28P01") ?? unreachable).conditionName() ?? unreachable, "invalid_password"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("08006") ?? unreachable).conditionName() ?? unreachable, "connection_failure"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("08003") ?? unreachable).conditionName() ?? unreachable, "connection_does_not_exist"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("08P01") ?? unreachable).conditionName() ?? unreachable, "protocol_violation"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("53300") ?? unreachable).conditionName() ?? unreachable, "too_many_connections"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("53200") ?? unreachable).conditionName() ?? unreachable, "out_of_memory"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("57014") ?? unreachable).conditionName() ?? unreachable, "query_canceled"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("57P01") ?? unreachable).conditionName() ?? unreachable, "admin_shutdown"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("57P02") ?? unreachable).conditionName() ?? unreachable, "crash_shutdown"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("57P03") ?? unreachable).conditionName() ?? unreachable, "cannot_connect_now"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("22P02") ?? unreachable).conditionName() ?? unreachable, "invalid_text_representation"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("22012") ?? unreachable).conditionName() ?? unreachable, "division_by_zero"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("25P02") ?? unreachable).conditionName() ?? unreachable, "in_failed_sql_transaction"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("58030") ?? unreachable).conditionName() ?? unreachable, "io_error"), true);
    expect_eq(std.mem.eql(u8, (SqlStateCode.parse("XX000") ?? unreachable).conditionName() ?? unreachable, "internal_error"), true);

    // 未知自定义 5 字符码返回 null
    expect_eq((SqlStateCode.parse("99999") ?? unreachable).conditionName() == null, true);
});

test("3. isRetryable accurately distinguishes transient vs permanent errors", () => {
    // 应自动重试
    expect_eq((SqlStateCode.parse("40001") ?? unreachable).isRetryable(), true);
    expect_eq((SqlStateCode.parse("40P01") ?? unreachable).isRetryable(), true);
    expect_eq((SqlStateCode.parse("08000") ?? unreachable).isRetryable(), true);
    expect_eq((SqlStateCode.parse("08006") ?? unreachable).isRetryable(), true);
    expect_eq((SqlStateCode.parse("53300") ?? unreachable).isRetryable(), true);
    expect_eq((SqlStateCode.parse("57P01") ?? unreachable).isRetryable(), true);
    expect_eq((SqlStateCode.parse("57P02") ?? unreachable).isRetryable(), true);
    expect_eq((SqlStateCode.parse("57P03") ?? unreachable).isRetryable(), true);

    // 不应盲目重试
    expect_eq((SqlStateCode.parse("23505") ?? unreachable).isRetryable(), false);
    expect_eq((SqlStateCode.parse("42601") ?? unreachable).isRetryable(), false);
    expect_eq((SqlStateCode.parse("28P01") ?? unreachable).isRetryable(), false);
    expect_eq((SqlStateCode.parse("57014") ?? unreachable).isRetryable(), false);
    expect_eq((SqlStateCode.parse("22P02") ?? unreachable).isRetryable(), false);
    expect_eq((SqlStateCode.parse("XX000") ?? unreachable).isRetryable(), false);
});

test("4. isConstraintViolation and isConnectionError predicates", () => {
    expect_eq((SqlStateCode.parse("23505") ?? unreachable).isConstraintViolation(), true);
    expect_eq((SqlStateCode.parse("23503") ?? unreachable).isConstraintViolation(), true);
    expect_eq((SqlStateCode.parse("23P01") ?? unreachable).isConstraintViolation(), true);
    expect_eq((SqlStateCode.parse("42601") ?? unreachable).isConstraintViolation(), false);

    expect_eq((SqlStateCode.parse("08001") ?? unreachable).isConnectionError(), true);
    expect_eq((SqlStateCode.parse("08006") ?? unreachable).isConnectionError(), true);
    expect_eq((SqlStateCode.parse("57P01") ?? unreachable).isConnectionError(), true);
    expect_eq((SqlStateCode.parse("57P02") ?? unreachable).isConnectionError(), true);
    expect_eq((SqlStateCode.parse("57P03") ?? unreachable).isConnectionError(), true);
    expect_eq((SqlStateCode.parse("57014") ?? unreachable).isConnectionError(), false);
    expect_eq((SqlStateCode.parse("40001") ?? unreachable).isConnectionError(), false);
});

test("5. parse rejects malformed codes and format writes safely without heap", () => {
    // 长度错误、小写字母、非法符号均拒绝
    expect_eq(SqlStateCode.parse("") == null, true);
    expect_eq(SqlStateCode.parse("2350") == null, true);
    expect_eq(SqlStateCode.parse("235050") == null, true);
    expect_eq(SqlStateCode.parse("23p01") == null, true);
    expect_eq(SqlStateCode.parse("xx000") == null, true);
    expect_eq(SqlStateCode.parse("23-05") == null, true);
    expect_eq(SqlStateCode.parse("2350 ") == null, true);

    const code = SqlStateCode.parse("23505") ?? unreachable;
    const same = SqlStateCode.parse("23505") ?? unreachable;
    const diff = SqlStateCode.parse("23503") ?? unreachable;
    expect_eq(code.eql(same), true);
    expect_eq(code.eql(diff), false);

    let small: [3]u8 = [0, 0, 0];
    expect_eq(code.format(&small) == null, true);

    let buf: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    const formatted = code.format(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, formatted, "23505"), true);
});
