// Source: Absorbed from /data/dev/bun/src/sql_jsc/mysql/MySQLStatement.cp | Synced: 2026-11-22T21:47:44Z
// Pure-CP zero-allocation MySQL prepared statement lifecycle, execution flags, and column deduplicator.
// Tracks COM_STMT_PREPARE_OK metadata progress (params_received / columns_received), per-execution
// flags, result row counts, error codes, and Bun's right-to-left duplicate column detection.

/// 单条 MySQL 预处理语句支持的最大结果列数（零堆分配内联上限）
pub const MYSQL_STMT_MAX_COLUMNS: usize = 64;

/// 单条 MySQL 预处理语句支持的最大参数元数据数（零堆分配内联上限）
pub const MYSQL_STMT_MAX_PARAMS: usize = 32;

/// `MySqlStmtLifecycle` 状态机与容量错误集
pub err MySqlStmtLifecycleError {
    TooManyColumns,
    TooManyParams,
    InvalidStateTransition,
}

/// MySQL 预处理语句生命周期状态（对齐 Bun `MySQLStatement.Status`）
pub enum MySqlStmtStatus {
    Pending,
    Parsing,
    Prepared,
    Failed,
}

/// MySQL 预处理语句单次执行标志位（对齐 Bun `@packed(u8) ExecutionFlags`）
pub struct MySqlStmtExecutionFlags {
    header_received: bool = false,
    needs_duplicate_check: bool = true,
    need_to_send_params: bool = true,
    columns_eof_received: bool = false,

    /// 打包为 4-bit 紧凑掩码字节
    fn toMask(): u8 {
        let m: u8 = 0;
        if (_.header_received) {
            m |= 0x01;
        }
        if (_.needs_duplicate_check) {
            m |= 0x02;
        }
        if (_.need_to_send_params) {
            m |= 0x04;
        }
        if (_.columns_eof_received) {
            m |= 0x08;
        }
        return m;
    }

    /// 从 4-bit 掩码恢复执行标志位
    static fn fromMask(mask: u8): MySqlStmtExecutionFlags {
        return MySqlStmtExecutionFlags{
            header_received: (mask & 0x01) != 0,
            needs_duplicate_check: (mask & 0x02) != 0,
            need_to_send_params: (mask & 0x04) != 0,
            columns_eof_received: (mask & 0x08) != 0,
        };
    }
}

/// 结果列标识符种类（命名列 / 数字索引列 / 被右侧覆盖的重复列）
pub enum MySqlStmtColumnKeyKind {
    Named,
    Indexed,
    Duplicate,
}

/// MySQL 预处理语句单列元数据（零拷贝借用列名切片）
pub struct MySqlStmtColumnMeta {
    key_kind: MySqlStmtColumnKeyKind = MySqlStmtColumnKeyKind.Named,
    name: []const u8 = "",
    index: u32 = 0,
    field_type: u8 = 0xFD,
    is_unsigned: bool = false,
    character_set: u16 = 45,

    /// 构造命名列元数据
    static fn named(name: []const u8, field_type: u8, is_unsigned: bool, character_set: u16): MySqlStmtColumnMeta {
        return MySqlStmtColumnMeta{
            key_kind: MySqlStmtColumnKeyKind.Named,
            name: name,
            index: 0,
            field_type: field_type,
            is_unsigned: is_unsigned,
            character_set: character_set,
        };
    }

    /// 构造数字索引列元数据
    static fn indexed(index: u32, field_type: u8, is_unsigned: bool, character_set: u16): MySqlStmtColumnMeta {
        return MySqlStmtColumnMeta{
            key_kind: MySqlStmtColumnKeyKind.Indexed,
            name: "",
            index: index,
            field_type: field_type,
            is_unsigned: is_unsigned,
            character_set: character_set,
        };
    }

    /// 该列是否因被右侧同名/同索引列覆盖而被标记为重复列
    fn isDuplicate(): bool {
        return _.key_kind == MySqlStmtColumnKeyKind.Duplicate;
    }
}

/// 结果列集合统计标志（对齐 Bun `SQLDataCell.Flags`）
pub struct MySqlStmtFieldsFlags {
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

/// MySQL 预处理语句生命周期、执行标志与列去重状态机（零动态堆分配）。
pub struct MySqlStmtLifecycle {
    statement_id: u32 = 0,
    status: MySqlStmtStatus = MySqlStmtStatus.Parsing,
    expected_params: u16 = 0,
    params_received: u32 = 0,
    expected_columns: u16 = 0,
    columns_received: u32 = 0,
    columns: [MYSQL_STMT_MAX_COLUMNS]MySqlStmtColumnMeta = [MySqlStmtColumnMeta{}; MYSQL_STMT_MAX_COLUMNS],
    execution_flags: MySqlStmtExecutionFlags = MySqlStmtExecutionFlags{},
    fields_flags: MySqlStmtFieldsFlags = MySqlStmtFieldsFlags{},
    result_count: u64 = 0,
    error_code: u16 = 0,

    /// 初始化 `Parsing` 状态的预处理语句状态机
    static fn init(): MySqlStmtLifecycle {
        return MySqlStmtLifecycle{};
    }

    /// 收到 `COM_STMT_PREPARE_OK` 响应后记录 `statement_id` 与预期参数/列数量。
    /// 若参数与列数均为 0，立即推进至 `MySqlStmtStatus.Prepared`。
    fn onPrepareOk(*_, statement_id: u32, num_columns: u16, num_params: u16): MySqlStmtLifecycleError!bool {
        if (_.status == MySqlStmtStatus.Failed) {
            return MySqlStmtLifecycleError.InvalidStateTransition;
        }
        if (@as(usize, num_columns) > MYSQL_STMT_MAX_COLUMNS) {
            return MySqlStmtLifecycleError.TooManyColumns;
        }
        if (@as(usize, num_params) > MYSQL_STMT_MAX_PARAMS) {
            return MySqlStmtLifecycleError.TooManyParams;
        }
        _.statement_id = statement_id;
        _.expected_columns = num_columns;
        _.expected_params = num_params;
        _.params_received = 0;
        _.columns_received = 0;
        if (num_columns == 0 && num_params == 0) {
            _.status = MySqlStmtStatus.Prepared;
        } else {
            _.status = MySqlStmtStatus.Parsing;
        }
        return _.status == MySqlStmtStatus.Prepared;
    }

    /// 收到一条参数 `ColumnDefinition41` 包后递增 `params_received`；
    /// 若参数与结果列均已收齐则自动进入 `Prepared`。
    fn onParamDefReceived(*_): MySqlStmtLifecycleError!u32 {
        if (_.status == MySqlStmtStatus.Failed) {
            return MySqlStmtLifecycleError.InvalidStateTransition;
        }
        if (_.params_received >= @as(u32, _.expected_params)) {
            return MySqlStmtLifecycleError.TooManyParams;
        }
        _.params_received += 1;
        if (_.params_received == @as(u32, _.expected_params) &&
            _.columns_received == @as(u32, _.expected_columns)) {
            * = _.checkForDuplicateFields();
            _.status = MySqlStmtStatus.Prepared;
        }
        return _.params_received;
    }

    /// 收到一条结果列 `ColumnDefinition41` 元数据后写入槽位并递增 `columns_received`；
    /// 在 `Parsing` 阶段若参数与结果列均已收齐则自动去重并进入 `Prepared`。
    fn onColumnDefReceived(*_, col: MySqlStmtColumnMeta): MySqlStmtLifecycleError!u32 {
        if (_.status == MySqlStmtStatus.Failed) {
            return MySqlStmtLifecycleError.InvalidStateTransition;
        }
        const idx = @as(usize, _.columns_received);
        if (idx >= MYSQL_STMT_MAX_COLUMNS || idx >= @as(usize, _.expected_columns)) {
            return MySqlStmtLifecycleError.TooManyColumns;
        }
        _.columns[idx] = col;
        _.columns_received += 1;
        _.execution_flags.needs_duplicate_check = true;
        if (_.status == MySqlStmtStatus.Parsing &&
            _.params_received == @as(u32, _.expected_params) &&
            _.columns_received == @as(u32, _.expected_columns)) {
            * = _.checkForDuplicateFields();
            _.status = MySqlStmtStatus.Prepared;
        }
        return _.columns_received;
    }

    /// 执行阶段收到 `ResultSetHeader` 后更新预期列数并置位 `header_received`
    fn onResultHeaderReceived(*_, num_columns: u16): MySqlStmtLifecycleError!bool {
        if (_.status != MySqlStmtStatus.Prepared) {
            return MySqlStmtLifecycleError.InvalidStateTransition;
        }
        if (@as(usize, num_columns) > MYSQL_STMT_MAX_COLUMNS) {
            return MySqlStmtLifecycleError.TooManyColumns;
        }
        _.execution_flags.header_received = true;
        _.execution_flags.columns_eof_received = false;
        _.execution_flags.needs_duplicate_check = true;
        _.expected_columns = num_columns;
        _.columns_received = 0;
        return true;
    }

    /// 收到列定义结束 EOF 帧后置位 `columns_eof_received` 并执行列去重
    fn onColumnsEof(*_): MySqlStmtFieldsFlags {
        _.execution_flags.columns_eof_received = true;
        return _.checkForDuplicateFields();
    }

    /// 记录收到一条二进制结果行，返回累计行数
    fn onRowReceived(*_): u64 {
        _.result_count += 1;
        return _.result_count;
    }

    /// 收到 MySQL `ERR_Packet` 错误码，进入 `Failed` 状态
    fn onError(*_, code: u16) {
        _.status = MySqlStmtStatus.Failed;
        _.error_code = code;
    }

    /// 按 Bun 右向左逆序去重算法扫描当前已收到的结果列（最右侧列胜出，左侧重复列标为 `Duplicate`）
    fn checkForDuplicateFields(*_): MySqlStmtFieldsFlags {
        if (!_.execution_flags.needs_duplicate_check) {
            return _.fields_flags;
        }
        _.execution_flags.needs_duplicate_check = false;

        const col_len = @as(usize, _.columns_received);
        let flags = MySqlStmtFieldsFlags{};
        let remaining: usize = col_len;
        while (remaining > 0) {
            remaining -= 1;
            const kind = _.columns[remaining].key_kind;
            if (kind == MySqlStmtColumnKeyKind.Duplicate) {
                flags.has_duplicate_columns = true;
            } else if (kind == MySqlStmtColumnKeyKind.Named) {
                flags.has_named_columns = true;
                const target_name = _.columns[remaining].name;
                let j: usize = remaining + 1;
                while (j < col_len) {
                    if (_.columns[j].key_kind == MySqlStmtColumnKeyKind.Named &&
                        bytesEql(_.columns[j].name, target_name)) {
                        _.columns[remaining].key_kind = MySqlStmtColumnKeyKind.Duplicate;
                        flags.has_duplicate_columns = true;
                        break;
                    }
                    j += 1;
                }
            } else if (kind == MySqlStmtColumnKeyKind.Indexed) {
                flags.has_indexed_columns = true;
                const target_idx = _.columns[remaining].index;
                let j: usize = remaining + 1;
                while (j < col_len) {
                    if (_.columns[j].key_kind == MySqlStmtColumnKeyKind.Indexed &&
                        _.columns[j].index == target_idx) {
                        _.columns[remaining].key_kind = MySqlStmtColumnKeyKind.Duplicate;
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

    /// 按下标查看列元数据
    fn columnAt(idx: usize): MySqlStmtColumnMeta? {
        if (idx >= @as(usize, _.columns_received)) {
            return null;
        }
        return _.columns[idx];
    }

    /// 统计去重后的唯一有效结果列数
    fn uniqueColumnCount(): usize {
        const col_len = @as(usize, _.columns_received);
        let count: usize = 0;
        let i: usize = 0;
        while (i < col_len) {
            if (!_.columns[i].isDuplicate()) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// 语句是否已就绪可执行 `COM_STMT_EXECUTE`
    fn isReadyForExecute(): bool {
        return _.status == MySqlStmtStatus.Prepared;
    }

    /// 重置单次执行状态（对齐 Bun `MySQLStatement.reset`），保留 `statement_id` 与 `Prepared` 状态供复用
    fn resetExecution(*_) {
        _.result_count = 0;
        _.columns_received = 0;
        _.execution_flags = MySqlStmtExecutionFlags{};
    }
}
