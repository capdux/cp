// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/FieldMessage.cp | Synced: 2026-11-22T04:20:02Z
// Pure-CP zero-allocation PostgreSQL v3.0 ErrorResponse / NoticeResponse 18-field diagnostic codec.

/// 单条 ErrorResponse / NoticeResponse 诊断消息允许的最大字段数
pub const PG_MAX_DIAGNOSTIC_FIELDS: usize = 24;

/// `PgDiagnosticFields` 编解码与校验错误集
pub err PgFieldMessageError {
    UnknownFieldType,
    Truncated,
    MissingTerminator,
    TooManyFields,
    TrailingBytes,
    BufferTooSmall,
}

/// PostgreSQL v3.0 ErrorResponse / NoticeResponse 完整 18 种字段标识字节（对齐 Bun `FieldType`）
pub enum PgDiagnosticFieldType: u8 {
    Severity = 'S',
    LocalizedSeverity = 'V',
    Code = 'C',
    Message = 'M',
    Detail = 'D',
    Hint = 'H',
    Position = 'P',
    InternalPosition = 'p',
    InternalQuery = 'q',
    Where = 'W',
    Schema = 's',
    Table = 't',
    Column = 'c',
    DataType = 'd',
    Constraint = 'n',
    File = 'F',
    Line = 'L',
    Routine = 'R',
}

/// 将单字节标识安全映射为已知 `PgDiagnosticFieldType`，未知字节返回 `null`
pub fn parseDiagnosticFieldType(code: u8): PgDiagnosticFieldType? {
    if (code == 'S') return PgDiagnosticFieldType.Severity;
    if (code == 'V') return PgDiagnosticFieldType.LocalizedSeverity;
    if (code == 'C') return PgDiagnosticFieldType.Code;
    if (code == 'M') return PgDiagnosticFieldType.Message;
    if (code == 'D') return PgDiagnosticFieldType.Detail;
    if (code == 'H') return PgDiagnosticFieldType.Hint;
    if (code == 'P') return PgDiagnosticFieldType.Position;
    if (code == 'p') return PgDiagnosticFieldType.InternalPosition;
    if (code == 'q') return PgDiagnosticFieldType.InternalQuery;
    if (code == 'W') return PgDiagnosticFieldType.Where;
    if (code == 's') return PgDiagnosticFieldType.Schema;
    if (code == 't') return PgDiagnosticFieldType.Table;
    if (code == 'c') return PgDiagnosticFieldType.Column;
    if (code == 'd') return PgDiagnosticFieldType.DataType;
    if (code == 'n') return PgDiagnosticFieldType.Constraint;
    if (code == 'F') return PgDiagnosticFieldType.File;
    if (code == 'L') return PgDiagnosticFieldType.Line;
    if (code == 'R') return PgDiagnosticFieldType.Routine;
    return null;
}

/// 返回字段类型的标准英文名称
pub fn diagnosticFieldName(ft: PgDiagnosticFieldType): []const u8 {
    if (ft == PgDiagnosticFieldType.Severity) return "severity";
    if (ft == PgDiagnosticFieldType.LocalizedSeverity) return "localized_severity";
    if (ft == PgDiagnosticFieldType.Code) return "code";
    if (ft == PgDiagnosticFieldType.Message) return "message";
    if (ft == PgDiagnosticFieldType.Detail) return "detail";
    if (ft == PgDiagnosticFieldType.Hint) return "hint";
    if (ft == PgDiagnosticFieldType.Position) return "position";
    if (ft == PgDiagnosticFieldType.InternalPosition) return "internal_position";
    if (ft == PgDiagnosticFieldType.InternalQuery) return "internal_query";
    if (ft == PgDiagnosticFieldType.Where) return "where";
    if (ft == PgDiagnosticFieldType.Schema) return "schema";
    if (ft == PgDiagnosticFieldType.Table) return "table";
    if (ft == PgDiagnosticFieldType.Column) return "column";
    if (ft == PgDiagnosticFieldType.DataType) return "datatype";
    if (ft == PgDiagnosticFieldType.Constraint) return "constraint";
    if (ft == PgDiagnosticFieldType.File) return "file";
    if (ft == PgDiagnosticFieldType.Line) return "line";
    return "routine";
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

/// 单条 PostgreSQL 诊断字段键值对
pub struct PgDiagnosticField {
    field_type: PgDiagnosticFieldType = PgDiagnosticFieldType.Message,
    value: []const u8 = "",

    fn eql(other: PgDiagnosticField): bool {
        return _.field_type == other.field_type && bytesEql(_.value, other.value);
    }
}

/// PostgreSQL ErrorResponse / NoticeResponse 全量字段容器（固定容量 24，零堆分配）
pub struct PgDiagnosticFields {
    items: [PG_MAX_DIAGNOSTIC_FIELDS]PgDiagnosticField = [PgDiagnosticField{}; PG_MAX_DIAGNOSTIC_FIELDS],
    len: usize = 0,

    static fn init(): PgDiagnosticFields {
        return PgDiagnosticFields{};
    }

    fn add(*_, field_type: PgDiagnosticFieldType, value: []const u8): usize! {
        if (_.len >= PG_MAX_DIAGNOSTIC_FIELDS) {
            return err.TooManyFields;
        }
        const idx = _.len;
        _.items[idx] = PgDiagnosticField{
            field_type: field_type,
            value: value,
        };
        _.len += 1;
        return idx;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): PgDiagnosticField? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }

    /// 按字段标识查找首个匹配值
    fn get(field_type: PgDiagnosticFieldType): ([]const u8)? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i].field_type == field_type) {
                return _.items[i].value;
            }
            i += 1;
        }
        return null;
    }

    /// 优先返回非本地化严重级别 `'V'`，否则回退 `'S'`
    fn severity(): ([]const u8)? {
        if (v := _.get(PgDiagnosticFieldType.LocalizedSeverity)) {
            return v;
        }
        return _.get(PgDiagnosticFieldType.Severity);
    }

    fn code(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Code);
    }

    fn message(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Message);
    }

    fn detail(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Detail);
    }

    fn hint(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Hint);
    }

    fn schema(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Schema);
    }

    fn table(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Table);
    }

    fn column(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Column);
    }

    fn constraint(): ([]const u8)? {
        return _.get(PgDiagnosticFieldType.Constraint);
    }

    /// 提取 5 字符 SQLSTATE 错误码的前 2 位分类前缀（如 `"23"`、`"42"`、`"40"`）
    fn sqlstateClass(): ([]const u8)? {
        const c = _.code() ?? return null;
        if (c.len < 2) {
            return null;
        }
        return c[0..2];
    }

    /// 判断是否为 PostgreSQL 经典可重试事务并发冲突（`40001` 序列化失败 / `40P01` 死锁检测）
    fn isRetryable(): bool {
        const c = _.code() ?? return false;
        return bytesEql(c, "40001") || bytesEql(c, "40P01");
    }

    fn encodedLen(): usize {
        let total: usize = 1; // 尾部 0x00 终结字节
        let i: usize = 0;
        while (i < _.len) {
            total += 1 + _.items[i].value.len + 1;
            i += 1;
        }
        return total;
    }

    /// 将所有字段编码为 PostgreSQL ErrorResponse/NoticeResponse 线协议载荷（`[tag][val\0]...[0x00]`）
    fn encodePayload(out: []u8): usize! {
        const need = _.encodedLen();
        if (out.len < need) {
            return err.BufferTooSmall;
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            out[pos] = @as(u8, @intFromEnum(_.items[i].field_type));
            pos += 1;
            const val = _.items[i].value;
            let j: usize = 0;
            while (j < val.len) {
                out[pos + j] = val[j];
                j += 1;
            }
            pos += val.len;
            out[pos] = 0;
            pos += 1;
            i += 1;
        }
        out[pos] = 0;
        return pos + 1;
    }

    /// 零拷贝解码线协议载荷（按 PostgreSQL v3.0 协议规范自动跳过未知未来扩展字段）
    static fn decodePayload(payload: []const u8): PgDiagnosticFields! {
        return decodePayloadInternal(payload, false);
    }

    /// 严格模式解码线协议载荷（遇到未在 18 种已知标识中的字段字节时返回 `UnknownFieldType`）
    static fn decodePayloadStrict(payload: []const u8): PgDiagnosticFields! {
        return decodePayloadInternal(payload, true);
    }

    fn eql(other: PgDiagnosticFields): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!_.items[i].eql(other.items[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

fn decodePayloadInternal(payload: []const u8, strict: bool): PgDiagnosticFields! {
    let out = PgDiagnosticFields.init();
    let pos: usize = 0;
    let saw_terminator = false;

    while (pos < payload.len) {
        const tag = payload[pos];
        pos += 1;
        if (tag == 0) {
            saw_terminator = true;
            break;
        }
        let nul_pos = pos;
        while (nul_pos < payload.len && payload[nul_pos] != 0) {
            nul_pos += 1;
        }
        if (nul_pos >= payload.len) {
            return err.Truncated;
        }
        const val = payload[pos..nul_pos];
        pos = nul_pos + 1;

        if (ft := parseDiagnosticFieldType(tag)) {
            * = out.add(ft, val)?;
        } else if (strict) {
            return err.UnknownFieldType;
        }
    }

    if (!saw_terminator) {
        return err.MissingTerminator;
    }
    if (pos != payload.len) {
        return err.TrailingBytes;
    }
    return out;
}
