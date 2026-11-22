// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/ReadyForQuery.cp | Synced: 2026-11-22T05:52:40Z
// ============================================================================
// Pure-CP Zero-Allocation PostgreSQL v3 Backend Control Frame Codec
// Absorbed from Bun: src/sql/postgres/protocol/{ReadyForQuery,BackendKeyData,CommandComplete,ParameterStatus,TransactionStatusIndicator}.cp
// ============================================================================

import { PgWireWriter } from "./pg_wire_writer.cp";
import { PgWireReader } from "./pg_wire_reader.cp";

/// PostgreSQL 后端控制消息类型标签
pub const PG_READY_FOR_QUERY_TAG: u8 = 'Z';
pub const PG_BACKEND_KEY_DATA_TAG: u8 = 'K';
pub const PG_COMMAND_COMPLETE_TAG: u8 = 'C';
pub const PG_PARAMETER_STATUS_TAG: u8 = 'S';

/// `BackendKeyData` 帧声明长度（含自身 4 字节：`4 + 4(pid) + 4(secret) = 12`）
pub const PG_BACKEND_KEY_DATA_FRAME_LEN: u32 = 12;

/// 后端控制帧编解码错误集
pub err PgControlError {
    BufferTooSmall,
    InvalidTag,
    InvalidFrameLength,
    InvalidTxStatus,
    TruncatedPayload,
    MissingNullTerminator,
    TrailingBytes,
}

/// PostgreSQL 事务状态指示符（对齐 Bun `TransactionStatusIndicator.cp`）
pub enum PgTxStatus: u8 {
    idle = 'I',
    in_transaction = 'T',
    failed_transaction = 'E',

    static fn fromByte(b: u8): PgTxStatus! {
        if (b == 'I') {
            return PgTxStatus.idle;
        }
        if (b == 'T') {
            return PgTxStatus.in_transaction;
        }
        if (b == 'E') {
            return PgTxStatus.failed_transaction;
        }
        return err.InvalidTxStatus;
    }

    fn toByte(): u8 {
        return switch (_) {
            .idle => 'I',
            .in_transaction => 'T',
            .failed_transaction => 'E',
        };
    }

    fn isIdle(): bool {
        return _ == PgTxStatus.idle;
    }

    fn inTransaction(): bool {
        return _ == PgTxStatus.in_transaction;
    }

    fn isFailed(): bool {
        return _ == PgTxStatus.failed_transaction;
    }
}

/// PostgreSQL `BackendKeyData` (`'K'`) 取消密钥对（用于后续 `CancelRequest`）
pub struct PgBackendKeyData {
    process_id: u32 = 0,
    secret_key: u32 = 0,
}

/// 零拷贝 `ParameterStatus` (`'S'`) 服务端运行时参数通知视图
pub struct PgParameterStatusView {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// 零拷贝 `CommandComplete` (`'C'`) 命令完成标签视图
pub struct PgCommandCompleteView {
    tag: []const u8 = "",

    /// 解析末尾影响/返回行数（如 `"INSERT 0 5"` -> `5`, `"UPDATE 3"` -> `3`, `"SELECT 10"` -> `10`；
    /// 无数字尾段如 `"BEGIN"` / `"COMMIT"` / `"CREATE TABLE"` 返回 `null`）
    fn rowsAffected(): u64? {
        if (_.tag.len == 0) {
            return null;
        }
        let end = _.tag.len;
        let start = end;
        while (start > 0 && _.tag[start - 1] >= '0' && _.tag[start - 1] <= '9') {
            start -= 1;
        }
        if (start == end) {
            return null;
        }
        // Must be preceded by space (e.g. "UPDATE 3", "INSERT 0 5")
        if (start == 0 || _.tag[start - 1] != ' ') {
            return null;
        }
        let val: u64 = 0;
        let i = start;
        while (i < end) {
            const digit: u64 = @as(u64, _.tag[i] - '0');
            if (val > 1844674407370955161) {
                return null;
            }
            val = val * 10 + digit;
            i += 1;
        }
        return val;
    }
}

/// 编码 PostgreSQL v3 `ReadyForQuery` (`'Z'`) 6 字节定长帧
pub fn encodePgReadyForQuery(buf: []u8, status: PgTxStatus): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_READY_FOR_QUERY_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(status.toByte()) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `ReadyForQuery` (`'Z'`) 6 字节定长帧
pub fn decodePgReadyForQuery(wire: []const u8): PgTxStatus! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_READY_FOR_QUERY_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    if (frame.payload.len != 1) {
        return err.InvalidFrameLength;
    }
    return PgTxStatus.fromByte(frame.payload[0]);
}

/// 编码 PostgreSQL v3 `BackendKeyData` (`'K'`) 13 字节定长帧
pub fn encodePgBackendKeyData(buf: []u8, key: PgBackendKeyData): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_BACKEND_KEY_DATA_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(key.process_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(key.secret_key) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `BackendKeyData` (`'K'`) 13 字节定长帧（严格校验载荷 8 字节）
pub fn decodePgBackendKeyData(wire: []const u8): PgBackendKeyData! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_BACKEND_KEY_DATA_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    if (frame.payload.len != 8) {
        return err.InvalidFrameLength;
    }
    let body = PgWireReader.init(frame.payload);
    const pid = body.readU32() catch {
        return err.TruncatedPayload;
    };
    const secret = body.readU32() catch {
        return err.TruncatedPayload;
    };
    return PgBackendKeyData{
        process_id: pid,
        secret_key: secret,
    };
}

/// 编码 PostgreSQL v3 `CommandComplete` (`'C'`) 帧
pub fn encodePgCommandComplete(buf: []u8, command_tag: []const u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_COMMAND_COMPLETE_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(command_tag) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `CommandComplete` (`'C'`) 帧（零拷贝）
pub fn decodePgCommandComplete(wire: []const u8): PgCommandCompleteView! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_COMMAND_COMPLETE_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const tag = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return PgCommandCompleteView{
        tag: tag,
    };
}

/// 编码 PostgreSQL v3 `ParameterStatus` (`'S'`) 帧
pub fn encodePgParameterStatus(buf: []u8, name: []const u8, value: []const u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_PARAMETER_STATUS_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(name) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(value) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `ParameterStatus` (`'S'`) 帧（零拷贝）
pub fn decodePgParameterStatus(wire: []const u8): PgParameterStatusView! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_PARAMETER_STATUS_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const name = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    const value = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return PgParameterStatusView{
        name: name,
        value: value,
    };
}
