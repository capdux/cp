// Source: Absorbed from /data/dev/bun/src/sql_jsc/postgres/PostgresSQLStatement.cp | Synced: 2026-11-22T21:45:30Z
// Pure-CP zero-allocation PostgreSQL prepared statement lifecycle & column deduplication engine.
// Tracks Pending -> Parsing -> Prepared/Failed transitions, parameter OIDs, SQLSTATE errors,
// and Bun's right-to-left duplicate column detection (last column wins, earlier duplicates tagged).

/// 单条预处理语句支持的最大结果列元数据数（零堆分配内联上限）
pub const PG_STMT_MAX_FIELDS: usize = 64;

/// 单条预处理语句支持的最大参数 OID 数（零堆分配内联上限）
pub const PG_STMT_MAX_PARAMS: usize = 32;

/// `PgStmtLifecycle` 状态机与容量错误集
pub err PgStmtLifecycleError {
    TooManyFields,
    TooManyParams,
    InvalidStateTransition,
}

/// PostgreSQL 预处理语句生命周期阶段（对齐 Bun `PostgresSQLStatement.Status`）
pub enum PgStmtStatus {
    Pending,
    Parsing,
    Prepared,
    Failed,

    /// 是否正处于解析阶段（对齐 Bun `StatementStatus.isRunning`）
    fn isRunning(): bool {
        return _ == PgStmtStatus.Parsing;
    }

    /// 是否已完成 `Parse` / `Describe` 可直接绑定执行
    fn isReady(): bool {
        return _ == PgStmtStatus.Prepared;
    }

    /// 是否已进入失败终态
    fn isFailed(): bool {
        return _ == PgStmtStatus.Failed;
    }
}

/// 结果列标识符类型（命名列 / 数字索引列 / 已被后方同名列覆盖的重复列）
pub enum PgStmtColumnKeyKind {
    Named,
    Indexed,
    Duplicate,
}

/// 结果集单列元数据描述符（零拷贝借用列名切片）
pub struct PgStmtColumnMeta {
    key_kind: PgStmtColumnKeyKind = PgStmtColumnKeyKind.Named,
    name: []const u8 = "",
    index: u32 = 0,
    type_oid: i32 = 0,

    /// 构造命名列元数据
    static fn named(name: []const u8, type_oid: i32): PgStmtColumnMeta {
        return PgStmtColumnMeta{
            key_kind: PgStmtColumnKeyKind.Named,
            name: name,
            index: 0,
            type_oid: type_oid,
        };
    }

    /// 构造数字索引列元数据（例如 `"0"`、`"1"`）
    static fn indexed(index: u32, type_oid: i32): PgStmtColumnMeta {
        return PgStmtColumnMeta{
            key_kind: PgStmtColumnKeyKind.Indexed,
            name: "",
            index: index,
            type_oid: type_oid,
        };
    }

    /// 该列是否被后方同名/同索引列覆盖而标记为重复列
    fn isDuplicate(): bool {
        return _.key_kind == PgStmtColumnKeyKind.Duplicate;
    }
}

/// 结果列集合统计标志（对齐 Bun `SQLDataCell.Flags`）
pub struct PgStmtFieldsFlags {
    has_duplicate_columns: bool = false,
    has_named_columns: bool = false,
    has_indexed_columns: bool = false,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// PostgreSQL 预处理语句生命周期与列去重状态机（零动态堆分配）。
pub struct PgStmtLifecycle {
    status: PgStmtStatus = PgStmtStatus.Pending,
    fields: [PG_STMT_MAX_FIELDS]PgStmtColumnMeta = [PgStmtColumnMeta{}; PG_STMT_MAX_FIELDS],
    field_count: usize = 0,
    param_oids: [PG_STMT_MAX_PARAMS]i32 = [0; PG_STMT_MAX_PARAMS],
    param_count: usize = 0,
    needs_duplicate_check: bool = true,
    fields_flags: PgStmtFieldsFlags = PgStmtFieldsFlags{},
    error_code: [5]u8 = [0; 5],
    has_error_code: bool = false,

    /// 初始化 `Pending` 状态的预处理语句生命周期对象
    static fn init(): PgStmtLifecycle {
        return PgStmtLifecycle{};
    }

    /// 进入 `Parsing` 阶段（仅允许从 `Pending` 发起）
    fn beginParse(*_): PgStmtLifecycleError!bool {
        if (_.status != PgStmtStatus.Pending) {
            return PgStmtLifecycleError.InvalidStateTransition;
        }
        _.status = PgStmtStatus.Parsing;
        return true;
    }

    /// 记录 `ParameterDescription` 返回的参数 OID 列表
    fn setParamOids(*_, oids: []const i32): PgStmtLifecycleError!usize {
        if (oids.len > PG_STMT_MAX_PARAMS) {
            return PgStmtLifecycleError.TooManyParams;
        }
        let i: usize = 0;
        while (i < oids.len) {
            _.param_oids[i] = oids[i];
            i += 1;
        }
        _.param_count = oids.len;
        return oids.len;
    }

    /// 追加一条 `RowDescription` 列元数据（自动置位 `needs_duplicate_check = true`）
    fn addField(*_, col: PgStmtColumnMeta): PgStmtLifecycleError!usize {
        if (_.field_count >= PG_STMT_MAX_FIELDS) {
            return PgStmtLifecycleError.TooManyFields;
        }
        _.fields[_.field_count] = col;
        _.field_count += 1;
        _.needs_duplicate_check = true;
        return _.field_count;
    }

    /// 按 Bun 右向左逆序去重算法扫描结果列：
    /// 同名或同索引列保留最右侧一列，所有左侧重复列标记为 `PgStmtColumnKeyKind.Duplicate`。
    /// 幂等：重复调用在未新增列时直接返回缓存的 `fields_flags`。
    fn checkForDuplicateFields(*_): PgStmtFieldsFlags {
        if (!_.needs_duplicate_check) {
            return _.fields_flags;
        }
        _.needs_duplicate_check = false;

        let flags = PgStmtFieldsFlags{};
        let remaining: usize = _.field_count;
        while (remaining > 0) {
            remaining -= 1;
            const kind = _.fields[remaining].key_kind;
            if (kind == PgStmtColumnKeyKind.Duplicate) {
                flags.has_duplicate_columns = true;
            } else if (kind == PgStmtColumnKeyKind.Named) {
                flags.has_named_columns = true;
                const target_name = _.fields[remaining].name;
                let j: usize = remaining + 1;
                while (j < _.field_count) {
                    if (_.fields[j].key_kind == PgStmtColumnKeyKind.Named &&
                        bytesEql(_.fields[j].name, target_name)) {
                        _.fields[remaining].key_kind = PgStmtColumnKeyKind.Duplicate;
                        flags.has_duplicate_columns = true;
                        break;
                    }
                    j += 1;
                }
            } else if (kind == PgStmtColumnKeyKind.Indexed) {
                flags.has_indexed_columns = true;
                const target_idx = _.fields[remaining].index;
                let j: usize = remaining + 1;
                while (j < _.field_count) {
                    if (_.fields[j].key_kind == PgStmtColumnKeyKind.Indexed &&
                        _.fields[j].index == target_idx) {
                        _.fields[remaining].key_kind = PgStmtColumnKeyKind.Duplicate;
                        flags.has_duplicate_columns = true;
                        break;
                    }
                    j += 1;
                }
            }
        }

        _.fields_flags = flags;
        return flags;
    }

    /// 标记语句解析完成（`Parsing` -> `Prepared`），并自动执行结果列去重检测
    fn markPrepared(*_): PgStmtLifecycleError!bool {
        if (_.status != PgStmtStatus.Parsing) {
            return PgStmtLifecycleError.InvalidStateTransition;
        }
        * = _.checkForDuplicateFields();
        _.status = PgStmtStatus.Prepared;
        return true;
    }

    /// 标记语句失败并可选记录 5 字节 PostgreSQL SQLSTATE（如 `"42601"`）
    fn markFailed(*_, sqlstate: []const u8) {
        _.status = PgStmtStatus.Failed;
        if (sqlstate.len >= 5) {
            let i: usize = 0;
            while (i < 5) {
                _.error_code[i] = sqlstate[i];
                i += 1;
            }
            _.has_error_code = true;
        } else {
            _.has_error_code = false;
        }
    }

    /// 返回记录的 5 字节 SQLSTATE（若未记录则返回 `null`）
    fn errorSqlState(): ([]const u8)? {
        if (!_.has_error_code) {
            return null;
        }
        return _.error_code[0..5];
    }

    /// 按下标查看结果列元数据
    fn fieldAt(idx: usize): PgStmtColumnMeta? {
        if (idx >= _.field_count) {
            return null;
        }
        return _.fields[idx];
    }

    /// 返回已记录的参数 OID 切片
    fn paramOidsSlice(): []const i32 {
        return _.param_oids[0.._.param_count];
    }

    /// 统计去重后有效（非 `Duplicate`）结果列数量
    fn uniqueFieldCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.field_count) {
            if (!_.fields[i].isDuplicate()) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// 重置生命周期状态机至初始 `Pending` 状态
    fn reset(*_) {
        _.status = PgStmtStatus.Pending;
        _.field_count = 0;
        _.param_count = 0;
        _.needs_duplicate_check = true;
        _.fields_flags = PgStmtFieldsFlags{};
        _.has_error_code = false;
    }
}
