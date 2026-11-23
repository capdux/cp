// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/ErrorResponse.cp | Synced: 2026-11-23T01:31:58Z
// ============================================================================
// Pure-CP Zero-Allocation PostgreSQL v3 ErrorResponse ('E') & NoticeResponse ('N') Codec
// Absorbed from Bun: src/sql/postgres/protocol/{ErrorResponse,NoticeResponse,FieldMessage,FieldType}.cp
// ============================================================================

import { PgWireWriter } from "./pgWriter.cp";
import { PgWireReader } from "./pgReader.cp";

/// PostgreSQL `ErrorResponse` 后端消息类型标签 (`'E' = 0x45`)
pub const PG_ERROR_RESPONSE_TAG: u8 = 'E';

/// PostgreSQL `NoticeResponse` 后端消息类型标签 (`'N' = 0x4E`)
pub const PG_NOTICE_RESPONSE_TAG: u8 = 'N';

/// 单帧诊断字段数量上限（PostgreSQL 规范共 18 种标准字段，上限 64 防异常大包）
pub const PG_MAX_DIAGNOSTIC_FIELDS: usize = 64;

/// `ErrorResponse` / `NoticeResponse` 编解码错误集
pub err PgErrorNoticeError {
    BufferTooSmall,
    InvalidTag,
    TruncatedPayload,
    MissingFieldTerminator,
    MissingNullTerminator,
    TooManyFields,
    TrailingBytes,
}

/// PostgreSQL v3 全部 18 种诊断字段标识（对齐 Bun `FieldType.cp`）
pub enum PgDiagnosticFieldKind: u8 {
    unknown = 0,
    severity = 'S',
    localized_severity = 'V',
    code = 'C',
    message = 'M',
    detail = 'D',
    hint = 'H',
    position = 'P',
    internal_position = 'p',
    internal_query = 'q',
    where = 'W',
    schema = 's',
    table = 't',
    column = 'c',
    datatype = 'd',
    constraint = 'n',
    file = 'F',
    line = 'L',
    routine = 'R',

    static fn fromByte(b: u8): PgDiagnosticFieldKind {
        return switch (b) {
            'S' => PgDiagnosticFieldKind.severity,
            'V' => PgDiagnosticFieldKind.localized_severity,
            'C' => PgDiagnosticFieldKind.code,
            'M' => PgDiagnosticFieldKind.message,
            'D' => PgDiagnosticFieldKind.detail,
            'H' => PgDiagnosticFieldKind.hint,
            'P' => PgDiagnosticFieldKind.position,
            'p' => PgDiagnosticFieldKind.internal_position,
            'q' => PgDiagnosticFieldKind.internal_query,
            'W' => PgDiagnosticFieldKind.where,
            's' => PgDiagnosticFieldKind.schema,
            't' => PgDiagnosticFieldKind.table,
            'c' => PgDiagnosticFieldKind.column,
            'd' => PgDiagnosticFieldKind.datatype,
            'n' => PgDiagnosticFieldKind.constraint,
            'F' => PgDiagnosticFieldKind.file,
            'L' => PgDiagnosticFieldKind.line,
            'R' => PgDiagnosticFieldKind.routine,
            else => PgDiagnosticFieldKind.unknown,
        };
    }

    fn codeByte(): u8 {
        return switch (_) {
            .unknown => 0,
            .severity => 'S',
            .localized_severity => 'V',
            .code => 'C',
            .message => 'M',
            .detail => 'D',
            .hint => 'H',
            .position => 'P',
            .internal_position => 'p',
            .internal_query => 'q',
            .where => 'W',
            .schema => 's',
            .table => 't',
            .column => 'c',
            .datatype => 'd',
            .constraint => 'n',
            .file => 'F',
            .line => 'L',
            .routine => 'R',
        };
    }
}

/// 单条零拷贝诊断字段记录（对齐 Bun `FieldMessage.cp`）
pub struct PgDiagnosticField {
    kind: PgDiagnosticFieldKind = PgDiagnosticFieldKind.unknown,
    raw_tag: u8 = 0,
    value: []const u8 = "",

    static fn of(kind: PgDiagnosticFieldKind, value: []const u8): PgDiagnosticField {
        return PgDiagnosticField{
            kind: kind,
            raw_tag: kind.codeByte(),
            value: value,
        };
    }

    static fn custom(raw_tag: u8, value: []const u8): PgDiagnosticField {
        return PgDiagnosticField{
            kind: PgDiagnosticFieldKind.fromByte(raw_tag),
            raw_tag: raw_tag,
            value: value,
        };
    }
}

/// 零堆分配流式诊断字段迭代器（遇到终止字节 `0x00` 结束并校验无尾随垃圾）
pub struct PgDiagnosticIter {
    reader: PgWireReader,
    is_error: bool = true,
    done: bool = false,
    seen: usize = 0,

    fn isError(): bool {
        return _.is_error;
    }

    fn isNotice(): bool {
        return !_.is_error;
    }

    fn next(*_): PgDiagnosticField?! {
        if (_.done) {
            return null;
        }
        const tag = _.reader.readByte() catch {
            return err.MissingFieldTerminator;
        };
        if (tag == 0) {
            _.done = true;
            if (_.reader.remaining() != 0) {
                return err.TrailingBytes;
            }
            return null;
        }
        if (_.seen >= PG_MAX_DIAGNOSTIC_FIELDS) {
            return err.TooManyFields;
        }
        const val = _.reader.readZ() catch {
            return err.MissingNullTerminator;
        };
        _.seen += 1;
        return PgDiagnosticField{
            kind: PgDiagnosticFieldKind.fromByte(tag),
            raw_tag: tag,
            value: val,
        };
    }
}

fn sliceEql(a: []const u8, b: []const u8): bool {
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

/// 零拷贝聚合诊断视图（聚合 PostgreSQL 全部 18 种标准 ErrorResponse / NoticeResponse 字段）
pub struct PgErrorNoticeView {
    is_error: bool = true,
    severity: []const u8 = "",
    localized_severity: []const u8 = "",
    sqlstate: []const u8 = "",
    message: []const u8 = "",
    detail: []const u8 = "",
    hint: []const u8 = "",
    position: []const u8 = "",
    internal_position: []const u8 = "",
    internal_query: []const u8 = "",
    where: []const u8 = "",
    schema: []const u8 = "",
    table: []const u8 = "",
    column: []const u8 = "",
    datatype: []const u8 = "",
    constraint: []const u8 = "",
    file: []const u8 = "",
    line: []const u8 = "",
    routine: []const u8 = "",
    field_count: usize = 0,

    /// 返回非本地化严重级别（`'V'` 优先，PostgreSQL < 9.6 回退至 `'S'`）
    fn effectiveSeverity(): []const u8 {
        if (_.localized_severity.len > 0) {
            return _.localized_severity;
        }
        return _.severity;
    }

    /// 判断是否为致命级错误（`FATAL` 或 `PANIC`）
    fn isFatal(): bool {
        const sev = _.effectiveSeverity();
        return sliceEql(sev, "FATAL") || sliceEql(sev, "PANIC");
    }

    /// 判断是否为完整性约束冲突（SQLSTATE Class `23`，如 `23505` unique_violation / `23503` foreign_key_violation）
    fn isConstraintViolation(): bool {
        return _.sqlstate.len >= 2 && _.sqlstate[0] == '2' && _.sqlstate[1] == '3';
    }

    /// 判断是否为典型可重试错误（`40001` 序列化冲突、`40P01` 死锁或 `08xxx` 连接异常）
    fn isRetryableSqlState(): bool {
        if (sliceEql(_.sqlstate, "40001") || sliceEql(_.sqlstate, "40P01")) {
            return true;
        }
        return _.sqlstate.len >= 2 && _.sqlstate[0] == '0' && _.sqlstate[1] == '8';
    }
}

fn encodeDiagnosticFrame(
    buf: []u8,
    frame_tag: u8,
    fields: []const PgDiagnosticField,
): ([]const u8)! {
    if (fields.len > PG_MAX_DIAGNOSTIC_FIELDS) {
        return err.TooManyFields;
    }
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(frame_tag) catch {
        return err.BufferTooSmall;
    };
    let i: usize = 0;
    while (i < fields.len) {
        const f = fields[i];
        i += 1;
        const tag_byte: u8 = if (f.raw_tag != 0) f.raw_tag else f.kind.codeByte();
        if (tag_byte == 0) {
            return err.InvalidTag;
        }
        * = w.writeByte(tag_byte) catch {
            return err.BufferTooSmall;
        };
        * = w.writeCString(f.value) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.writeByte(0) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 编码 PostgreSQL v3 `ErrorResponse` (`'E'`) 后端诊断帧（零堆分配）
pub fn encodePgErrorResponse(
    buf: []u8,
    fields: []const PgDiagnosticField,
): ([]const u8)! {
    return encodeDiagnosticFrame(buf, PG_ERROR_RESPONSE_TAG, fields);
}

/// 编码 PostgreSQL v3 `NoticeResponse` (`'N'`) 后端诊断帧（零堆分配）
pub fn encodePgNoticeResponse(
    buf: []u8,
    fields: []const PgDiagnosticField,
): ([]const u8)! {
    return encodeDiagnosticFrame(buf, PG_NOTICE_RESPONSE_TAG, fields);
}

/// 解码 `ErrorResponse` (`'E'`) 或 `NoticeResponse` (`'N'`) 为零堆分配流式字段迭代器
pub fn decodePgErrorNoticeIter(wire: []const u8): PgDiagnosticIter! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_ERROR_RESPONSE_TAG && frame.tag != PG_NOTICE_RESPONSE_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    if (frame.payload.len == 0) {
        return err.MissingFieldTerminator;
    }
    return PgDiagnosticIter{
        reader: PgWireReader.init(frame.payload),
        is_error: frame.tag == PG_ERROR_RESPONSE_TAG,
        done: false,
        seen: 0,
    };
}

/// 解码 `ErrorResponse` (`'E'`) 或 `NoticeResponse` (`'N'`) 为零拷贝聚合诊断视图
pub fn decodePgErrorOrNotice(wire: []const u8): PgErrorNoticeView! {
    let it = decodePgErrorNoticeIter(wire)?;
    let view = PgErrorNoticeView{
        is_error: it.isError(),
    };
    while (f := it.next()?) {
        view.field_count += 1;
        switch (f.kind) {
            .severity => { view.severity = f.value; },
            .localized_severity => { view.localized_severity = f.value; },
            .code => { view.sqlstate = f.value; },
            .message => { view.message = f.value; },
            .detail => { view.detail = f.value; },
            .hint => { view.hint = f.value; },
            .position => { view.position = f.value; },
            .internal_position => { view.internal_position = f.value; },
            .internal_query => { view.internal_query = f.value; },
            .where => { view.where = f.value; },
            .schema => { view.schema = f.value; },
            .table => { view.table = f.value; },
            .column => { view.column = f.value; },
            .datatype => { view.datatype = f.value; },
            .constraint => { view.constraint = f.value; },
            .file => { view.file = f.value; },
            .line => { view.line = f.value; },
            .routine => { view.routine = f.value; },
            .unknown => {},
        }
    }
    return view;
}

/// 严格解码 PostgreSQL v3 `ErrorResponse` (`'E'`) 帧（若为 `'N'` 或其他 Tag 返回 `InvalidTag`）
pub fn decodePgErrorResponse(wire: []const u8): PgErrorNoticeView! {
    const view = decodePgErrorOrNotice(wire)?;
    if (!view.is_error) {
        return err.InvalidTag;
    }
    return view;
}

/// 严格解码 PostgreSQL v3 `NoticeResponse` (`'N'`) 帧（若为 `'E'` 或其他 Tag 返回 `InvalidTag`）
pub fn decodePgNoticeResponse(wire: []const u8): PgErrorNoticeView! {
    const view = decodePgErrorOrNotice(wire)?;
    if (view.is_error) {
        return err.InvalidTag;
    }
    return view;
}
