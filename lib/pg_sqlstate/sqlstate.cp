// Source: Absorbed from /data/dev/bun/src/sql/postgres/AnyPostgresError.cp

/// PostgreSQL SQLSTATE 前 2 字符大类分类
export enum SqlStateClass {
    success,
    warning,
    no_data,
    connection_exception,
    data_exception,
    integrity_constraint_violation,
    invalid_transaction_state,
    invalid_authorization,
    transaction_rollback,
    syntax_or_access_error,
    insufficient_resources,
    operator_intervention,
    system_error,
    internal_error,
    other,

    /// 返回该大类对应的 2 字符 SQLSTATE 前缀
    fn prefix(): []const u8 {
        return switch (_) {
            .success => "00",
            .warning => "01",
            .no_data => "02",
            .connection_exception => "08",
            .data_exception => "22",
            .integrity_constraint_violation => "23",
            .invalid_transaction_state => "25",
            .invalid_authorization => "28",
            .transaction_rollback => "40",
            .syntax_or_access_error => "42",
            .insufficient_resources => "53",
            .operator_intervention => "57",
            .system_error => "58",
            .internal_error => "XX",
            .other => "??",
        };
    }
}

fn isValidSqlStateByte(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z');
}

/// PostgreSQL 5 字符 SQLSTATE 错误码（纯 CP、零外部依赖、零堆分配）
export struct SqlStateCode {
    bytes: [5]u8;

    /// 严格解析 5 字符 SQLSTATE 码（仅允许大写字母 A-Z 与数字 0-9）
    fn parse(code: []const u8): SqlStateCode? {
        if (code.len != 5) {
            return null;
        }
        let i: usize = 0;
        while (i < 5) {
            if (!isValidSqlStateByte(code[i])) {
                return null;
            }
            i += 1;
        }
        return SqlStateCode{
            bytes: [code[0], code[1], code[2], code[3], code[4]],
        };
    }

    /// 将 5 字节错误码写入调用方缓冲区并返回切片
    fn format(out: []u8): ([]const u8)? {
        if (out.len < 5) {
            return null;
        }
        let i: usize = 0;
        while (i < 5) {
            out[i] = _.bytes[i];
            i += 1;
        }
        return out[0..5];
    }

    /// 判断是否与指定 5 字符字面量完全一致
    fn eqlStr(s: []const u8): bool {
        if (s.len != 5) {
            return false;
        }
        return _.bytes[0] == s[0]
            && _.bytes[1] == s[1]
            && _.bytes[2] == s[2]
            && _.bytes[3] == s[3]
            && _.bytes[4] == s[4];
    }

    /// 判断两个 SqlStateCode 是否相等
    fn eql(other: SqlStateCode): bool {
        return _.bytes[0] == other.bytes[0]
            && _.bytes[1] == other.bytes[1]
            && _.bytes[2] == other.bytes[2]
            && _.bytes[3] == other.bytes[3]
            && _.bytes[4] == other.bytes[4];
    }

    /// 返回前 2 字符对应的 SQLSTATE 类别
    fn class(): SqlStateClass {
        const c0 = _.bytes[0];
        const c1 = _.bytes[1];
        if (c0 == '0' && c1 == '0') return SqlStateClass.success;
        if (c0 == '0' && c1 == '1') return SqlStateClass.warning;
        if (c0 == '0' && c1 == '2') return SqlStateClass.no_data;
        if (c0 == '0' && c1 == '8') return SqlStateClass.connection_exception;
        if (c0 == '2' && c1 == '2') return SqlStateClass.data_exception;
        if (c0 == '2' && c1 == '3') return SqlStateClass.integrity_constraint_violation;
        if (c0 == '2' && c1 == '5') return SqlStateClass.invalid_transaction_state;
        if (c0 == '2' && c1 == '8') return SqlStateClass.invalid_authorization;
        if (c0 == '4' && c1 == '0') return SqlStateClass.transaction_rollback;
        if (c0 == '4' && c1 == '2') return SqlStateClass.syntax_or_access_error;
        if (c0 == '5' && c1 == '3') return SqlStateClass.insufficient_resources;
        if (c0 == '5' && c1 == '7') return SqlStateClass.operator_intervention;
        if (c0 == '5' && c1 == '8') return SqlStateClass.system_error;
        if (c0 == 'X' && c1 == 'X') return SqlStateClass.internal_error;
        return SqlStateClass.other;
    }

    /// 返回 PostgreSQL 官方标准条件名称（覆盖 43 个高频核心码）
    fn conditionName(): ([]const u8)? {
        // Class 00 / 01 / 02
        if (_.eqlStr("00000")) return "successful_completion";
        if (_.eqlStr("01000")) return "warning";
        if (_.eqlStr("02000")) return "no_data";

        // Class 08 — Connection Exception
        if (_.eqlStr("08000")) return "connection_exception";
        if (_.eqlStr("08001")) return "sqlclient_unable_to_establish_sqlconnection";
        if (_.eqlStr("08003")) return "connection_does_not_exist";
        if (_.eqlStr("08004")) return "sqlserver_rejected_establishment_of_sqlconnection";
        if (_.eqlStr("08006")) return "connection_failure";
        if (_.eqlStr("08P01")) return "protocol_violation";

        // Class 22 — Data Exception
        if (_.eqlStr("22001")) return "string_data_right_truncation";
        if (_.eqlStr("22003")) return "numeric_value_out_of_range";
        if (_.eqlStr("22007")) return "invalid_datetime_format";
        if (_.eqlStr("22012")) return "division_by_zero";
        if (_.eqlStr("22P02")) return "invalid_text_representation";

        // Class 23 — Integrity Constraint Violation
        if (_.eqlStr("23000")) return "integrity_constraint_violation";
        if (_.eqlStr("23001")) return "restrict_violation";
        if (_.eqlStr("23502")) return "not_null_violation";
        if (_.eqlStr("23503")) return "foreign_key_violation";
        if (_.eqlStr("23505")) return "unique_violation";
        if (_.eqlStr("23514")) return "check_violation";
        if (_.eqlStr("23P01")) return "exclusion_violation";

        // Class 25 — Invalid Transaction State
        if (_.eqlStr("25000")) return "invalid_transaction_state";
        if (_.eqlStr("25006")) return "read_only_sql_transaction";
        if (_.eqlStr("25P02")) return "in_failed_sql_transaction";

        // Class 28 — Invalid Authorization Specification
        if (_.eqlStr("28000")) return "invalid_authorization_specification";
        if (_.eqlStr("28P01")) return "invalid_password";

        // Class 40 — Transaction Rollback
        if (_.eqlStr("40001")) return "serialization_failure";
        if (_.eqlStr("40P01")) return "deadlock_detected";

        // Class 42 — Syntax Error or Access Rule Violation
        if (_.eqlStr("42501")) return "insufficient_privilege";
        if (_.eqlStr("42601")) return "syntax_error";
        if (_.eqlStr("42703")) return "undefined_column";
        if (_.eqlStr("42P01")) return "undefined_table";
        if (_.eqlStr("42P07")) return "duplicate_table";

        // Class 53 — Insufficient Resources
        if (_.eqlStr("53100")) return "disk_full";
        if (_.eqlStr("53200")) return "out_of_memory";
        if (_.eqlStr("53300")) return "too_many_connections";

        // Class 57 — Operator Intervention
        if (_.eqlStr("57014")) return "query_canceled";
        if (_.eqlStr("57P01")) return "admin_shutdown";
        if (_.eqlStr("57P02")) return "crash_shutdown";
        if (_.eqlStr("57P03")) return "cannot_connect_now";

        // Class 58 — System Error
        if (_.eqlStr("58030")) return "io_error";

        // Class XX — Internal Error
        if (_.eqlStr("XX000")) return "internal_error";
        if (_.eqlStr("XX001")) return "data_corrupted";

        return null;
    }

    /// 是否为完整性约束冲突（Class 23xxx）
    fn isConstraintViolation(): bool {
        return _.class() == SqlStateClass.integrity_constraint_violation;
    }

    /// 是否为连接层或服务端停机恢复类错误（08xxx / 57P01 / 57P02 / 57P03）
    fn isConnectionError(): bool {
        if (_.class() == SqlStateClass.connection_exception) {
            return true;
        }
        return _.eqlStr("57P01") || _.eqlStr("57P02") || _.eqlStr("57P03");
    }

    /// 是否适合在事务/连接层自动重试（40001 / 40P01 / 08xxx / 53300 / 57P01-03）
    fn isRetryable(): bool {
        if (_.isConnectionError()) {
            return true;
        }
        return _.eqlStr("40001") || _.eqlStr("40P01") || _.eqlStr("53300");
    }

    /// 是否为成功完成类（Class 00xxx）
    fn isSuccess(): bool {
        return _.class() == SqlStateClass.success;
    }
}
