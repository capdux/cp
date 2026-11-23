// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/SASLInitialResponse.cp | Synced: 2026-11-23T01:32:28Z
// ============================================================================
// Pure-CP Zero-Allocation PostgreSQL v3 Auth (SASL/Password), COPY Stream & Terminate Codec
// Absorbed from Bun: src/sql/postgres/protocol/{SASLInitialResponse,SASLResponse,PasswordMessage,CopyData,CopyFail}.cp
// ============================================================================

import { PgWireWriter } from "./pgWriter.cp";
import { PgWireReader } from "./pgReader.cp";

/// PostgreSQL 鉴权、COPY 流与连接终止消息类型标签
pub const PG_PASSWORD_TAG: u8 = 'p';
pub const PG_COPY_DATA_TAG: u8 = 'd';
pub const PG_COPY_DONE_TAG: u8 = 'c';
pub const PG_COPY_FAIL_TAG: u8 = 'f';
pub const PG_TERMINATE_TAG: u8 = 'X';

/// 鉴权与 COPY 流编解码错误集
pub err PgAuthCopyError {
    BufferTooSmall,
    InvalidTag,
    InvalidFrameLength,
    TruncatedPayload,
    MissingNullTerminator,
    EmptyMechanism,
    TrailingBytes,
}

/// 零拷贝 `SASLInitialResponse` 视图（`data == null` 表示线协议 `-1` 无初始客户端数据）
pub struct PgSaslInitialResponseView {
    mechanism: []const u8 = "",
    data: ([]const u8)? = null,
}

/// 编码 PostgreSQL v3 `PasswordMessage` (`'p'`)（明文或 `md5...` 哈希）
pub fn encodePgPasswordMessage(buf: []u8, password: []const u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_PASSWORD_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(password) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `PasswordMessage` (`'p'`)（零拷贝）
pub fn decodePgPasswordMessage(wire: []const u8): ([]const u8)! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_PASSWORD_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const pw = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return pw;
}

/// 编码 PostgreSQL v3 `SASLInitialResponse` (`'p'`)（如 `SCRAM-SHA-256` 首次握手响应）
/// 线格式：`'p'` + `len(i32 BE)` + `mechanism\0` + `(-1 | data_len(i32 BE) + data)`
pub fn encodePgSaslInitialResponse(
    buf: []u8,
    mechanism: []const u8,
    initial_data: ([]const u8)?,
): ([]const u8)! {
    if (mechanism.len == 0) {
        return err.EmptyMechanism;
    }
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_PASSWORD_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(mechanism) catch {
        return err.BufferTooSmall;
    };
    if (d := initial_data) {
        * = w.writeLengthPrefixed(d) catch {
            return err.BufferTooSmall;
        };
    } else {
        * = w.writeNull() catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `SASLInitialResponse` (`'p'`)（零拷贝）
pub fn decodePgSaslInitialResponse(wire: []const u8): PgSaslInitialResponseView! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_PASSWORD_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const mech = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    if (mech.len == 0) {
        return err.EmptyMechanism;
    }
    const maybe_data = body.readLengthPrefixedOrNull() catch {
        return err.TruncatedPayload;
    };
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return PgSaslInitialResponseView{
        mechanism: mech,
        data: maybe_data,
    };
}

/// 编码 PostgreSQL v3 `SASLResponse` (`'p'`)（后续 SCRAM 客户端最终消息）
pub fn encodePgSaslResponse(buf: []u8, data: []const u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_PASSWORD_TAG) catch {
        return err.BufferTooSmall;
    };
    if (data.len > 0) {
        * = w.writeBytes(data) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `SASLResponse` (`'p'`)（零拷贝）
pub fn decodePgSaslResponse(wire: []const u8): ([]const u8)! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_PASSWORD_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    return frame.payload;
}

/// 编码 PostgreSQL v3 `CopyData` (`'d'`) 数据分片帧（支持二进制与文本载荷）
pub fn encodePgCopyData(buf: []u8, data: []const u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_COPY_DATA_TAG) catch {
        return err.BufferTooSmall;
    };
    if (data.len > 0) {
        * = w.writeBytes(data) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `CopyData` (`'d'`) 数据分片帧（零拷贝）
pub fn decodePgCopyData(wire: []const u8): ([]const u8)! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_COPY_DATA_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    return frame.payload;
}

/// 编码 PostgreSQL v3 `CopyDone` (`'c'`) 5 字节定长结束帧
pub fn encodePgCopyDone(buf: []u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_COPY_DONE_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 校验 PostgreSQL v3 `CopyDone` (`'c'`) 5 字节定长帧
pub fn decodePgCopyDone(wire: []const u8): bool! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_COPY_DONE_TAG) {
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

/// 编码 PostgreSQL v3 `CopyFail` (`'f'`) 错误中止帧
pub fn encodePgCopyFail(buf: []u8, message: []const u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_COPY_FAIL_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(message) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 PostgreSQL v3 `CopyFail` (`'f'`) 错误中止帧（零拷贝）
pub fn decodePgCopyFail(wire: []const u8): ([]const u8)! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_COPY_FAIL_TAG) {
        return err.InvalidTag;
    }
    if (r.remaining() != 0) {
        return err.TrailingBytes;
    }
    let body = PgWireReader.init(frame.payload);
    const msg = body.readZ() catch {
        return err.MissingNullTerminator;
    };
    if (body.remaining() != 0) {
        return err.TrailingBytes;
    }
    return msg;
}

/// 编码 PostgreSQL v3 `Terminate` (`'X'`) 5 字节定长优雅断开帧
pub fn encodePgTerminate(buf: []u8): ([]const u8)! {
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_TERMINATE_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 校验 PostgreSQL v3 `Terminate` (`'X'`) 5 字节定长帧
pub fn decodePgTerminate(wire: []const u8): bool! {
    let r = PgWireReader.init(wire);
    const frame = r.readTaggedFrame() catch {
        return err.TruncatedPayload;
    };
    if (frame.tag != PG_TERMINATE_TAG) {
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
