// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Parse.cp | Synced: 2026-11-22T05:37:21Z
// ============================================================================
// Pure-CP Zero-Allocation PostgreSQL v3 Extended Query Protocol Codec
// Absorbed from Bun: src/sql/postgres/protocol/{Parse,Describe,Execute,Close,PortalOrPreparedStatement}.cp
// ============================================================================

import { PgWireWriter } from "./pg_wire_writer.cp";
import { PgWireReader } from "./pg_wire_reader.cp";

/// PostgreSQL Extended Query 前端消息类型标签
pub const PG_PARSE_TAG: u8 = 'P';
pub const PG_DESCRIBE_TAG: u8 = 'D';
pub const PG_EXECUTE_TAG: u8 = 'E';
pub const PG_CLOSE_TAG: u8 = 'C';
pub const PG_SYNC_TAG: u8 = 'S';
pub const PG_FLUSH_TAG: u8 = 'H';

/// `Describe` / `Close` 目标对象字节标签（Portal `'P'` 或 Prepared Statement `'S'`）
pub const PG_TARGET_PORTAL: u8 = 'P';
pub const PG_TARGET_STATEMENT: u8 = 'S';

/// `Parse` 消息参数 OID 计数上限（PostgreSQL 线协议 16 位无符号计数上限 65535）
pub const PG_MAX_PARSE_PARAMS: usize = 65535;

/// PostgreSQL Extended Query 编解码错误集
pub err PgExtendedError {
    BufferTooSmall,
    TooManyParameters,
    InvalidTag,
    InvalidTargetKind,
    InvalidFrameLength,
    TruncatedPayload,
    MissingNullTerminator,
    NegativeMaxRows,
    TrailingBytes,
}

/// `Describe` / `Close` 目标类型（对齐 Bun `PortalOrPreparedStatement` 标签语义）
pub enum PgTargetKind: u8 {
    portal = 'P',
    prepared_statement = 'S',

    static fn fromByte(b: u8): PgTargetKind! {
        if (b == PG_TARGET_PORTAL) {
            return PgTargetKind.portal;
        }
        if (b == PG_TARGET_STATEMENT) {
            return PgTargetKind.prepared_statement;
        }
        return err.InvalidTargetKind;
    }

    fn isPortal(): bool {
        return _ == PgTargetKind.portal;
    }

    fn isStatement(): bool {
        return _ == PgTargetKind.prepared_statement;
    }
}

/// 零拷贝 Portal 或 Prepared Statement 引用（对齐 Bun `PortalOrPreparedStatement`）
pub struct PgTargetRef {
    kind: PgTargetKind = PgTargetKind.prepared_statement,
    name: []const u8 = "",

    static fn portal(name: []const u8): PgTargetRef {
        return PgTargetRef{
            kind: PgTargetKind.portal,
            name: name,
        };
    }

    static fn statement(name: []const u8): PgTargetRef {
        return PgTargetRef{
            kind: PgTargetKind.prepared_statement,
            name: name,
        };
    }

    /// 返回 PostgreSQL 线协议单字节目标标识（`'P'` 或 `'S'`）
    fn tag(): u8 {
        if (_.kind.isPortal()) {
            return PG_TARGET_PORTAL;
        }
        return PG_TARGET_STATEMENT;
    }

    /// 判断是否为匿名目标（空名称 `""` 对应 PostgreSQL unnamed statement/portal）
    fn isUnnamed(): bool {
        return _.name.len == 0;
    }
}

/// 零堆分配流式 `Parse` 参数类型 OID 迭代器
pub struct PgParamOidIter {
    reader: PgWireReader,
    total: usize = 0,
    index: usize = 0,

    fn count(): usize {
        return _.total;
    }

    fn remaining(): usize {
        if (_.index >= _.total) {
            return 0;
        }
        return _.total - _.index;
    }

    fn next(*_): i32?! {
        if (_.index >= _.total) {
            return null;
        }
        const oid = _.reader.readI32() catch {
            return err.TruncatedPayload;
        };
        _.index += 1;
        return oid;
    }
}

/// 零拷贝解码后的 PostgreSQL `Parse` (`'P'`) 消息视图
pub struct PgDecodedParse {
    name: []const u8 = "",
    query: []const u8 = "",
    params: PgParamOidIter,

    fn isUnnamed(): bool {
        return _.name.len == 0;
    }

    fn paramCount(): usize {
        return _.params.count();
    }
}

/// PostgreSQL `Execute` (`'E'`) 消息数据模型（`max_rows == 0` 表示不限行数）
pub struct PgExecuteMsg {
    portal: []const u8 = "",
    max_rows: i32 = 0,

    fn isUnnamedPortal(): bool {
        return _.portal.len == 0;
    }

    fn isUnlimited(): bool {
        return _.max_rows == 0;
    }
}

/// 编码 PostgreSQL v3 `Parse` (`'P'`) 前端消息帧（零堆分配）
/// 线格式：`'P'` + `len(i32 BE)` + `name\0` + `query\0` + `param_count(u16 BE)` + `param_oids[i32 BE]...`
pub fn encodePgParse(
    buf: []u8,
    name: []const u8,
    query: []const u8,
    param_oids: []const i32,
): ([]const u8)! {
    if (param_oids.len > PG_MAX_PARSE_PARAMS) {
        return err.TooManyParameters;
    }
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_PARSE_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(name) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(query) catch {
        return err.BufferTooSmall;
    };
    * = w.writeShort(param_oids.len) catch {
        return err.BufferTooSmall;
    };
    let i: usize = 0;
    while (i < param_oids.len) {
        * = w.writeI32(param_oids[i]) catch {
            return err.BufferTooSmall;
        };
        i += 1;
    }
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `Parse` (`'P'`) 前端消息帧（零拷贝，严格校验帧长与尾随字节）
pub fn decodePgParse(wire: []const u8): PgDecodedParse! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_PARSE_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const name = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    const query = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    const param_count_u16 = body.readU16() catch {
        return err.TruncatedPayload;
    };
    const param_count: usize = @as(usize, param_count_u16);
    const expected_bytes = param_count * 4;
    if (body.remaining() < expected_bytes) {
        return err.TruncatedPayload;
    }
    if (body.remaining() > expected_bytes) {
        return err.TrailingBytes;
    }
    return PgDecodedParse{
        name: name,
        query: query,
        params: PgParamOidIter{
            reader: body,
            total: param_count,
            index: 0,
        },
    };
}

/// 编码 PostgreSQL v3 `Describe` (`'D'`) 前端消息帧（零堆分配）
/// 线格式：`'D'` + `len(i32 BE)` + `('S' | 'P')` + `name\0`
pub fn encodePgDescribe(buf: []u8, target: PgTargetRef): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_DESCRIBE_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(target.tag()) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(target.name) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `Describe` (`'D'`) 前端消息帧（零拷贝）
pub fn decodePgDescribe(wire: []const u8): PgTargetRef! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_DESCRIBE_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const kind_byte = body.readByte() catch {
        return err.TruncatedPayload;
    };
    const kind = PgTargetKind.fromByte(kind_byte)?;
    const name = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return PgTargetRef{
        kind: kind,
        name: name,
    };
}

/// 编码 PostgreSQL v3 `Execute` (`'E'`) 前端消息帧（零堆分配）
/// 线格式：`'E'` + `len(i32 BE)` + `portal\0` + `max_rows(i32 BE)`
pub fn encodePgExecute(buf: []u8, portal: []const u8, max_rows: i32): ([]const u8)! {
    if (max_rows < 0) {
        return err.NegativeMaxRows;
    }
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_EXECUTE_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(portal) catch {
        return err.BufferTooSmall;
    };
    * = w.writeI32(max_rows) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `Execute` (`'E'`) 前端消息帧（零拷贝）
pub fn decodePgExecute(wire: []const u8): PgExecuteMsg! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_EXECUTE_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const portal = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    const max_rows = body.readI32() catch {
        return err.TruncatedPayload;
    };
    if (max_rows < 0) {
        return err.NegativeMaxRows;
    }
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return PgExecuteMsg{
        portal: portal,
        max_rows: max_rows,
    };
}

/// 编码 PostgreSQL v3 `Close` (`'C'`) 前端消息帧（零堆分配）
/// 线格式：`'C'` + `len(i32 BE)` + `('S' | 'P')` + `name\0`
pub fn encodePgClose(buf: []u8, target: PgTargetRef): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_CLOSE_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(target.tag()) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(target.name) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `Close` (`'C'`) 前端消息帧（零拷贝）
pub fn decodePgClose(wire: []const u8): PgTargetRef! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_CLOSE_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const kind_byte = body.readByte() catch {
        return err.TruncatedPayload;
    };
    const kind = PgTargetKind.fromByte(kind_byte)?;
    const name = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return PgTargetRef{
        kind: kind,
        name: name,
    };
}

/// 编码 PostgreSQL v3 `Sync` (`'S'`) 5 字节定长同步帧（触发服务端事务错误恢复与 `ReadyForQuery`）
pub fn encodePgSync(buf: []u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_SYNC_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 校验 PostgreSQL v3 `Sync` (`'S'`) 5 字节定长帧
pub fn decodePgSync(wire: []const u8): bool! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_SYNC_TAG) {
        return err.InvalidTag;
    }
    if (frame.payload.len != 0) {
        return err.InvalidFrameLength;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    return true;
}

/// 编码 PostgreSQL v3 `Flush` (`'H'`) 5 字节定长刷盘帧（用于管线化查询强制刷新服务端输出缓冲）
pub fn encodePgFlush(buf: []u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_FLUSH_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 校验 PostgreSQL v3 `Flush` (`'H'`) 5 字节定长帧
pub fn decodePgFlush(wire: []const u8): bool! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_FLUSH_TAG) {
        return err.InvalidTag;
    }
    if (frame.payload.len != 0) {
        return err.InvalidFrameLength;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    return true;
}
