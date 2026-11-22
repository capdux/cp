// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/OKPacket.cp | Synced: 2026-11-22T05:35:00Z
// ============================================================================
// Pure-CP Zero-Allocation MySQL OK_Packet (0x00/0xFE), EOF_Packet (0xFE) & StatusFlags Codec
// Absorbed from Bun: src/sql/mysql/protocol/{OKPacket,EOFPacket}.cp & StatusFlags.cp
// ============================================================================

import {
    MYSQL_PACKET_HEADER_SIZE,
    MySqlWireWriter,
} from "./mysqlWriter.cp";
import { MySqlWireReader } from "./mysqlReader.cp";

/// MySQL `OK_Packet` 头部标识字节 (`0x00`)
pub const MYSQL_OK_HEADER: u8 = 0x00;

/// MySQL `EOF_Packet` / `CLIENT_DEPRECATE_EOF` 结束包标识字节 (`0xFE`)
pub const MYSQL_EOF_HEADER: u8 = 0xfe;

/// MySQL 服务端 16 位状态位掩码常量（对齐 Bun `StatusFlags.cp`）
pub const MYSQL_FLAG_IN_TRANS: u16 = 1;
pub const MYSQL_FLAG_AUTOCOMMIT: u16 = 2;
pub const MYSQL_FLAG_MORE_RESULTS_EXISTS: u16 = 8;
pub const MYSQL_FLAG_NO_GOOD_INDEX_USED: u16 = 16;
pub const MYSQL_FLAG_NO_INDEX_USED: u16 = 32;
pub const MYSQL_FLAG_CURSOR_EXISTS: u16 = 64;
pub const MYSQL_FLAG_LAST_ROW_SENT: u16 = 128;
pub const MYSQL_FLAG_DB_DROPPED: u16 = 256;
pub const MYSQL_FLAG_NO_BACKSLASH_ESCAPES: u16 = 512;
pub const MYSQL_FLAG_METADATA_CHANGED: u16 = 1024;
pub const MYSQL_FLAG_QUERY_WAS_SLOW: u16 = 2048;
pub const MYSQL_FLAG_PS_OUT_PARAMS: u16 = 4096;
pub const MYSQL_FLAG_IN_TRANS_READONLY: u16 = 8192;
pub const MYSQL_FLAG_SESSION_STATE_CHANGED: u16 = 16384;

/// `OK_Packet` / `EOF_Packet` 编解码错误集
pub err MySqlOkEofError {
    BufferTooSmall,
    InvalidPacket,
    TruncatedPayload,
    InvalidOkHeader,
    InvalidEofHeader,
    InvalidEofLength,
}

/// MySQL 16 位服务器状态位掩码操作视图
pub struct MySqlServerStatusFlags {
    raw: u16 = 0,

    static fn fromInt(v: u16): MySqlServerStatusFlags {
        return MySqlServerStatusFlags{ raw: v };
    }

    fn toInt(): u16 {
        return _.raw;
    }

    fn has(mask: u16): bool {
        return (_.raw & mask) != 0;
    }

    fn withFlag(mask: u16): MySqlServerStatusFlags {
        return MySqlServerStatusFlags{ raw: _.raw | mask };
    }

    fn inTransaction(): bool {
        return _.has(MYSQL_FLAG_IN_TRANS);
    }

    fn isAutoCommit(): bool {
        return _.has(MYSQL_FLAG_AUTOCOMMIT);
    }

    fn hasMoreResults(): bool {
        return _.has(MYSQL_FLAG_MORE_RESULTS_EXISTS);
    }

    fn wasSlowQuery(): bool {
        return _.has(MYSQL_FLAG_QUERY_WAS_SLOW);
    }

    fn hasSessionStateChanged(): bool {
        return _.has(MYSQL_FLAG_SESSION_STATE_CHANGED);
    }
}

/// 零拷贝 MySQL `OK_Packet` 数据模型（兼容 `0x00` 与 `CLIENT_DEPRECATE_EOF` 下的 `0xFE`）
pub struct MySqlOkPacketView {
    sequence_id: u8 = 0,
    header: u8 = MYSQL_OK_HEADER,
    affected_rows: u64 = 0,
    last_insert_id: u64 = 0,
    status_flags: MySqlServerStatusFlags = MySqlServerStatusFlags{},
    warnings: u16 = 0,
    info: []const u8 = "",
}

/// 零拷贝 MySQL `EOF_Packet` 数据模型（固定 5 字节载荷：`0xFE` + `warnings(u16 LE)` + `status_flags(u16 LE)`）
pub struct MySqlEofPacketView {
    sequence_id: u8 = 0,
    header: u8 = MYSQL_EOF_HEADER,
    warnings: u16 = 0,
    status_flags: MySqlServerStatusFlags = MySqlServerStatusFlags{},
}

/// 编码标准带 4 字节包头的 MySQL `OK_Packet`（零堆分配）
pub fn encodeMySqlOkPacket(buf: []u8, pkt: MySqlOkPacketView): []const u8! {
    if (pkt.header != MYSQL_OK_HEADER && pkt.header != MYSQL_EOF_HEADER) {
        return err.InvalidOkHeader;
    }
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(pkt.sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(pkt.header) catch {
        return err.BufferTooSmall;
    };
    * = w.writeLenEncInt(pkt.affected_rows) catch {
        return err.BufferTooSmall;
    };
    * = w.writeLenEncInt(pkt.last_insert_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU16(pkt.status_flags.toInt()) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU16(pkt.warnings) catch {
        return err.BufferTooSmall;
    };
    if (pkt.info.len > 0) {
        * = w.writeBytes(pkt.info) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 零拷贝解码带 4 字节包头的 MySQL `OK_Packet`（支持 `0x00` 与 `0xFE`）
pub fn decodeMySqlOkPacket(packet: []const u8): MySqlOkPacketView! {
    if (packet.len < MYSQL_PACKET_HEADER_SIZE + 7) {
        return err.InvalidPacket;
    }
    let r = MySqlWireReader.init(packet);
    const payload_len: usize = @intCast(r.readU24() catch {
        return err.InvalidPacket;
    });
    const seq_id = r.readByte() catch {
        return err.InvalidPacket;
    };
    if (payload_len < 7 || r.remaining() < payload_len) {
        return err.TruncatedPayload;
    }
    const payload = r.readBytes(payload_len) catch {
        return err.TruncatedPayload;
    };

    let pr = MySqlWireReader.init(payload);
    const hdr = pr.readByte() catch {
        return err.TruncatedPayload;
    };
    if (hdr != MYSQL_OK_HEADER && hdr != MYSQL_EOF_HEADER) {
        return err.InvalidOkHeader;
    }
    const affected = pr.readLenEncInt() catch {
        return err.TruncatedPayload;
    };
    const last_id = pr.readLenEncInt() catch {
        return err.TruncatedPayload;
    };
    const status_raw = pr.readU16() catch {
        return err.TruncatedPayload;
    };
    const warnings = pr.readU16() catch {
        return err.TruncatedPayload;
    };
    const info = pr.peek();

    return MySqlOkPacketView{
        sequence_id: seq_id,
        header: hdr,
        affected_rows: affected,
        last_insert_id: last_id,
        status_flags: MySqlServerStatusFlags.fromInt(status_raw),
        warnings: warnings,
        info: info,
    };
}

/// 编码标准带 4 字节包头的 MySQL 4.1 `EOF_Packet`（4B 包头 + 5B 载荷 = 9 字节），零堆分配
pub fn encodeMySqlEofPacket(buf: []u8, pkt: MySqlEofPacketView): []const u8! {
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(pkt.sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(MYSQL_EOF_HEADER) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU16(pkt.warnings) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU16(pkt.status_flags.toInt()) catch {
        return err.BufferTooSmall;
    };
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 零拷贝解码标准带 4 字节包头的 MySQL 4.1 `EOF_Packet`（要求 `header == 0xFE` 且载荷长度为 5..8 字节）
pub fn decodeMySqlEofPacket(packet: []const u8): MySqlEofPacketView! {
    if (packet.len < MYSQL_PACKET_HEADER_SIZE + 5) {
        return err.InvalidPacket;
    }
    let r = MySqlWireReader.init(packet);
    const payload_len: usize = @intCast(r.readU24() catch {
        return err.InvalidPacket;
    });
    const seq_id = r.readByte() catch {
        return err.InvalidPacket;
    };
    if (payload_len < 5 || payload_len >= 9) {
        return err.InvalidEofLength;
    }
    if (r.remaining() < payload_len) {
        return err.TruncatedPayload;
    }
    const payload = r.readBytes(payload_len) catch {
        return err.TruncatedPayload;
    };

    let pr = MySqlWireReader.init(payload);
    const hdr = pr.readByte() catch {
        return err.TruncatedPayload;
    };
    if (hdr != MYSQL_EOF_HEADER) {
        return err.InvalidEofHeader;
    }
    const warnings = pr.readU16() catch {
        return err.TruncatedPayload;
    };
    const status_raw = pr.readU16() catch {
        return err.TruncatedPayload;
    };

    return MySqlEofPacketView{
        sequence_id: seq_id,
        header: hdr,
        warnings: warnings,
        status_flags: MySqlServerStatusFlags.fromInt(status_raw),
    };
}
