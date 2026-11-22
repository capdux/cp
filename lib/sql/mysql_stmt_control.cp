// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/StmtPrepareOKPacket.cp | Synced: 2026-11-22T05:59:52Z
// ============================================================================
// Pure-CP Zero-Allocation MySQL 4.1/8.0 Command Header, COM_STMT_PREPARE_OK & ResultSetHeader Codec
// Absorbed from Bun: src/sql/mysql/protocol/{CommandType,PacketHeader,StmtPrepareOKPacket,ResultSetHeader}.cp
// ============================================================================

import {
    MYSQL_PACKET_HEADER_SIZE,
    MYSQL_MAX_PACKET_PAYLOAD,
    MySqlWireWriter,
} from "./mysql_wire_writer.cp";
import { MySqlWireReader } from "./mysql_wire_reader.cp";

/// `COM_STMT_PREPARE_OK` 响应状态字节 (`0x00`)
pub const MYSQL_STMT_PREPARE_OK_STATUS: u8 = 0x00;

/// `COM_STMT_PREPARE_OK` 最小载荷长度（不含 `warning_count` 时的 10 字节）
pub const MYSQL_STMT_PREPARE_OK_MIN_LEN: usize = 10;

/// `COM_STMT_PREPARE_OK` 标准载荷长度（含 2 字节 `warning_count` 的 12 字节）
pub const MYSQL_STMT_PREPARE_OK_STD_LEN: usize = 12;

/// MySQL 命令与预处理响应编解码错误集
pub err MySqlStmtControlError {
    BufferTooSmall,
    TruncatedPayload,
    InvalidPacket,
    InvalidPrepareOkStatus,
    InvalidResultSetHeader,
    UnknownCommand,
    InvalidStatementIdFrame,
    TrailingBytes,
}

/// 完整 31 种 MySQL 客户端线协议命令枚举（对齐 Bun `CommandType.cp`）
pub enum MySqlWireCmd: u8 {
    com_quit = 0x01,
    com_init_db = 0x02,
    com_query = 0x03,
    com_field_list = 0x04,
    com_create_db = 0x05,
    com_drop_db = 0x06,
    com_refresh = 0x07,
    com_shutdown = 0x08,
    com_statistics = 0x09,
    com_process_info = 0x0a,
    com_connect = 0x0b,
    com_process_kill = 0x0c,
    com_debug = 0x0d,
    com_ping = 0x0e,
    com_time = 0x0f,
    com_delayed_insert = 0x10,
    com_change_user = 0x11,
    com_binlog_dump = 0x12,
    com_table_dump = 0x13,
    com_connect_out = 0x14,
    com_register_slave = 0x15,
    com_stmt_prepare = 0x16,
    com_stmt_execute = 0x17,
    com_stmt_send_long_data = 0x18,
    com_stmt_close = 0x19,
    com_stmt_reset = 0x1a,
    com_set_option = 0x1b,
    com_stmt_fetch = 0x1c,
    com_daemon = 0x1d,
    com_binlog_dump_gtid = 0x1e,
    com_reset_connection = 0x1f,

    static fn fromByte(b: u8): MySqlWireCmd! {
        return switch (b) {
            0x01 => MySqlWireCmd.com_quit,
            0x02 => MySqlWireCmd.com_init_db,
            0x03 => MySqlWireCmd.com_query,
            0x04 => MySqlWireCmd.com_field_list,
            0x05 => MySqlWireCmd.com_create_db,
            0x06 => MySqlWireCmd.com_drop_db,
            0x07 => MySqlWireCmd.com_refresh,
            0x08 => MySqlWireCmd.com_shutdown,
            0x09 => MySqlWireCmd.com_statistics,
            0x0a => MySqlWireCmd.com_process_info,
            0x0b => MySqlWireCmd.com_connect,
            0x0c => MySqlWireCmd.com_process_kill,
            0x0d => MySqlWireCmd.com_debug,
            0x0e => MySqlWireCmd.com_ping,
            0x0f => MySqlWireCmd.com_time,
            0x10 => MySqlWireCmd.com_delayed_insert,
            0x11 => MySqlWireCmd.com_change_user,
            0x12 => MySqlWireCmd.com_binlog_dump,
            0x13 => MySqlWireCmd.com_table_dump,
            0x14 => MySqlWireCmd.com_connect_out,
            0x15 => MySqlWireCmd.com_register_slave,
            0x16 => MySqlWireCmd.com_stmt_prepare,
            0x17 => MySqlWireCmd.com_stmt_execute,
            0x18 => MySqlWireCmd.com_stmt_send_long_data,
            0x19 => MySqlWireCmd.com_stmt_close,
            0x1a => MySqlWireCmd.com_stmt_reset,
            0x1b => MySqlWireCmd.com_set_option,
            0x1c => MySqlWireCmd.com_stmt_fetch,
            0x1d => MySqlWireCmd.com_daemon,
            0x1e => MySqlWireCmd.com_binlog_dump_gtid,
            0x1f => MySqlWireCmd.com_reset_connection,
            else => err.UnknownCommand,
        };
    }

    fn toByte(): u8 {
        return @intFromEnum(_);
    }

    /// 判断是否为二进制预处理协议命令 (`COM_STMT_*`)
    fn isPreparedCommand(): bool {
        const b = _.toByte();
        return (b >= 0x16 && b <= 0x1a) || b == 0x1c;
    }

    /// 判断是否为会话生命周期控制命令 (`COM_QUIT` / `COM_PING` / `COM_RESET_CONNECTION` / `COM_CHANGE_USER`)
    fn isConnectionControl(): bool {
        return _ == MySqlWireCmd.com_quit ||
            _ == MySqlWireCmd.com_ping ||
            _ == MySqlWireCmd.com_reset_connection ||
            _ == MySqlWireCmd.com_change_user;
    }
}

/// MySQL 4 字节包头（24 位小端载荷长度 + 8 位序号，对齐 Bun `PacketHeader.cp`）
pub struct MySqlWirePacketHeader {
    length: u32 = 0,
    sequence_id: u8 = 0,

    static fn decode(bytes: []const u8): MySqlWirePacketHeader? {
        if (bytes.len < MYSQL_PACKET_HEADER_SIZE) {
            return null;
        }
        const len_val: u32 = @as(u32, bytes[0]) |
            (@as(u32, bytes[1]) << 8) |
            (@as(u32, bytes[2]) << 16);
        return MySqlWirePacketHeader{
            length: len_val,
            sequence_id: bytes[3],
        };
    }

    fn encode(): [4]u8 {
        return [
            @intCast(_.length & 0xff),
            @intCast((_.length >> 8) & 0xff),
            @intCast((_.length >> 16) & 0xff),
            _.sequence_id,
        ];
    }

    /// 当载荷长度达到 `0xFFFFFF` (16 MiB - 1) 时，MySQL 协议要求后续紧跟下一个分片包
    fn isContinuationChunk(): bool {
        return _.length == MYSQL_MAX_PACKET_PAYLOAD;
    }
}

/// `COM_STMT_PREPARE_OK` 响应解码视图（对齐 Bun `StmtPrepareOKPacket.cp`）
pub struct MySqlStmtPrepareOkView {
    sequence_id: u8 = 0,
    statement_id: u32 = 0,
    num_columns: u16 = 0,
    num_params: u16 = 0,
    warning_count: u16 = 0,
    has_warning_count: bool = true,
}

/// `ResultSetHeader` 响应解码视图（对齐 Bun `ResultSetHeader.cp` + 可选 `CLIENT_OPTIONAL_RESULTSET_METADATA`）
pub struct MySqlResultSetHeaderView {
    sequence_id: u8 = 0,
    field_count: u64 = 0,
    metadata_follows: bool = true,
}

/// 零拷贝通用单命令帧视图（1 字节命令码 + 零拷贝载荷切片）
pub struct MySqlSimpleCommandView {
    sequence_id: u8 = 0,
    command: MySqlWireCmd = MySqlWireCmd.com_ping,
    arg: []const u8 = "",
}

/// 编码带 4 字节包头的 `COM_STMT_PREPARE_OK` 响应帧（零堆分配）
pub fn encodeMySqlStmtPrepareOk(buf: []u8, pkt: MySqlStmtPrepareOkView): ([]const u8)! {
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(pkt.sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(MYSQL_STMT_PREPARE_OK_STATUS) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(pkt.statement_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU16(pkt.num_columns) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU16(pkt.num_params) catch {
        return err.BufferTooSmall;
    };
    // 1-byte reserved filler (always 0x00)
    * = w.writeByte(0) catch {
        return err.BufferTooSmall;
    };
    if (pkt.has_warning_count) {
        * = w.writeU16(pkt.warning_count) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码带 4 字节包头的 `COM_STMT_PREPARE_OK` 响应帧（兼容 10B 与 12B 标准载荷）
pub fn decodeMySqlStmtPrepareOk(wire: []const u8): MySqlStmtPrepareOkView! {
    const hdr = MySqlWirePacketHeader.decode(wire) ?? {
        return err.TruncatedPayload;
    };
    const payload_len: usize = @as(usize, hdr.length);
    const total = MYSQL_PACKET_HEADER_SIZE + payload_len;
    if (wire.len < total) {
        return err.TruncatedPayload;
    }
    if (wire.len > total) {
        return err.TrailingBytes;
    }
    if (payload_len < MYSQL_STMT_PREPARE_OK_MIN_LEN) {
        return err.TruncatedPayload;
    }
    let r = MySqlWireReader.init(wire[MYSQL_PACKET_HEADER_SIZE..total]);
    const status = r.readByte() catch {
        return err.TruncatedPayload;
    };
    if (status != MYSQL_STMT_PREPARE_OK_STATUS) {
        return err.InvalidPrepareOkStatus;
    }
    const stmt_id = r.readU32() catch {
        return err.TruncatedPayload;
    };
    const num_cols = r.readU16() catch {
        return err.TruncatedPayload;
    };
    const num_params = r.readU16() catch {
        return err.TruncatedPayload;
    };
    // 1-byte reserved filler
    * = r.readByte() catch {
        return err.TruncatedPayload;
    };
    let warnings: u16 = 0;
    let has_warnings = false;
    if (payload_len >= MYSQL_STMT_PREPARE_OK_STD_LEN) {
        warnings = r.readU16() catch {
            return err.TruncatedPayload;
        };
        has_warnings = true;
    }
    return MySqlStmtPrepareOkView{
        sequence_id: hdr.sequence_id,
        statement_id: stmt_id,
        num_columns: num_cols,
        num_params: num_params,
        warning_count: warnings,
        has_warning_count: has_warnings,
    };
}

/// 编码带 4 字节包头的 `ResultSetHeader` 帧（`field_count` 必须 > 0，否则会与 `OK_Packet` 0x00 冲突）
pub fn encodeMySqlResultSetHeader(
    buf: []u8,
    sequence_id: u8,
    field_count: u64,
    include_metadata_flag: bool,
    metadata_follows: bool,
): ([]const u8)! {
    if (field_count == 0) {
        return err.InvalidResultSetHeader;
    }
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeLenEncInt(field_count) catch {
        return err.BufferTooSmall;
    };
    if (include_metadata_flag) {
        const flag_b: u8 = if (metadata_follows) 1 else 0;
        * = w.writeByte(flag_b) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码带 4 字节包头的 `ResultSetHeader` 帧（支持标准列数头及可选 1 字节 `metadata_follows` 标志）
pub fn decodeMySqlResultSetHeader(wire: []const u8): MySqlResultSetHeaderView! {
    const hdr = MySqlWirePacketHeader.decode(wire) ?? {
        return err.TruncatedPayload;
    };
    const payload_len: usize = @as(usize, hdr.length);
    const total = MYSQL_PACKET_HEADER_SIZE + payload_len;
    if (wire.len < total) {
        return err.TruncatedPayload;
    }
    if (wire.len > total) {
        return err.TrailingBytes;
    }
    if (payload_len == 0) {
        return err.TruncatedPayload;
    }
    let r = MySqlWireReader.init(wire[MYSQL_PACKET_HEADER_SIZE..total]);
    const field_count = r.readLenEncInt() catch {
        return err.InvalidResultSetHeader;
    };
    if (field_count == 0) {
        return err.InvalidResultSetHeader;
    }
    let meta_follows = true;
    if (r.remaining() == 1) {
        const flag_b = r.readByte() catch {
            return err.TruncatedPayload;
        };
        if (flag_b > 1) {
            return err.InvalidResultSetHeader;
        }
        meta_follows = flag_b == 1;
    } else if (r.remaining() > 1) {
        return err.TrailingBytes;
    }
    return MySqlResultSetHeaderView{
        sequence_id: hdr.sequence_id,
        field_count: field_count,
        metadata_follows: meta_follows,
    };
}

/// 编码通用 MySQL 客户端命令帧（如 `COM_PING` / `COM_QUIT` / `COM_INIT_DB` / `COM_STMT_PREPARE`）
pub fn encodeMySqlSimpleCommand(
    buf: []u8,
    sequence_id: u8,
    cmd: MySqlWireCmd,
    arg: []const u8,
): ([]const u8)! {
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(cmd.toByte()) catch {
        return err.BufferTooSmall;
    };
    if (arg.len > 0) {
        * = w.writeBytes(arg) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码通用 MySQL 客户端命令帧（零拷贝）
pub fn decodeMySqlSimpleCommand(wire: []const u8): MySqlSimpleCommandView! {
    const hdr = MySqlWirePacketHeader.decode(wire) ?? {
        return err.TruncatedPayload;
    };
    const payload_len: usize = @as(usize, hdr.length);
    const total = MYSQL_PACKET_HEADER_SIZE + payload_len;
    if (wire.len < total) {
        return err.TruncatedPayload;
    }
    if (wire.len > total) {
        return err.TrailingBytes;
    }
    if (payload_len < 1) {
        return err.TruncatedPayload;
    }
    const cmd = MySqlWireCmd.fromByte(wire[MYSQL_PACKET_HEADER_SIZE])?;
    return MySqlSimpleCommandView{
        sequence_id: hdr.sequence_id,
        command: cmd,
        arg: wire[MYSQL_PACKET_HEADER_SIZE + 1 .. total],
    };
}

/// 编码带 4 字节小端 `statement_id` 的预处理控制帧（适用于 `COM_STMT_CLOSE` / `COM_STMT_RESET`）
pub fn encodeMySqlStmtIdCommand(
    buf: []u8,
    sequence_id: u8,
    cmd: MySqlWireCmd,
    statement_id: u32,
): ([]const u8)! {
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(cmd.toByte()) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(statement_id) catch {
        return err.BufferTooSmall;
    };
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码 5 字节载荷预处理句柄命令帧，返回 `statement_id`
pub fn decodeMySqlStmtIdCommand(wire: []const u8, expected_cmd: MySqlWireCmd): u32! {
    const view = decodeMySqlSimpleCommand(wire)?;
    if (view.command != expected_cmd) {
        return err.InvalidStatementIdFrame;
    }
    if (view.arg.len != 4) {
        return err.InvalidStatementIdFrame;
    }
    let r = MySqlWireReader.init(view.arg);
    const stmt_id = r.readU32() catch {
        return err.TruncatedPayload;
    };
    return stmt_id;
}
