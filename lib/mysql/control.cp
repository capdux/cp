// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/HandshakeV10.cp | Synced: 2026-11-21T16:59:39Z

pub const CAP_CLIENT_SSL: u32 = 1 << 11;
pub const CAP_CLIENT_PLUGIN_AUTH: u32 = 1 << 19;

pub err Error {
    TruncatedPacket,
    UnsupportedProtocolVersion,
    InvalidOKPacket,
    InvalidErrorPacket,
    InvalidEOFPacket,
    InvalidAuthSwitchRequest,
    InvalidLengthEncodedInt,
}

struct Cursor {
    buf: string,
    pos: usize = 0,

    fn remaining(): usize {
        return _.buf.len - _.pos;
    }

    fn rest(): string {
        return _.buf[_.pos..];
    }

    fn readU8(*_): u8! {
        if (_.pos >= _.buf.len) {
            return err.TruncatedPacket;
        }
        const v = _.buf[_.pos];
        _.pos += 1;
        return v;
    }

    fn readU16LE(*_): u16! {
        if (_.remaining() < 2) {
            return err.TruncatedPacket;
        }
        const v: u16 = (_.buf[_.pos] as u16) | ((_.buf[_.pos + 1] as u16) << 8);
        _.pos += 2;
        return v;
    }

    fn readU24LE(*_): u32! {
        if (_.remaining() < 3) {
            return err.TruncatedPacket;
        }
        const v: u32 = (_.buf[_.pos] as u32) |
            ((_.buf[_.pos + 1] as u32) << 8) |
            ((_.buf[_.pos + 2] as u32) << 16);
        _.pos += 3;
        return v;
    }

    fn readU32LE(*_): u32! {
        if (_.remaining() < 4) {
            return err.TruncatedPacket;
        }
        const v: u32 = (_.buf[_.pos] as u32) |
            ((_.buf[_.pos + 1] as u32) << 8) |
            ((_.buf[_.pos + 2] as u32) << 16) |
            ((_.buf[_.pos + 3] as u32) << 24);
        _.pos += 4;
        return v;
    }

    fn readU64LE(*_): u64! {
        if (_.remaining() < 8) {
            return err.TruncatedPacket;
        }
        const lo = _.readU32LE()? as u64;
        const hi = _.readU32LE()? as u64;
        return lo | (hi << 32);
    }

    fn readBytes(*_, n: usize): ([]const u8)! {
        if (_.remaining() < n) {
            return err.TruncatedPacket;
        }
        const slice = _.buf[_.pos .. _.pos + n];
        _.pos += n;
        return slice;
    }

    fn skip(*_, n: usize)! {
        if (_.remaining() < n) {
            return err.TruncatedPacket;
        }
        _.pos += n;
    }

    fn readNulTerminated(*_): string! {
        let i = _.pos;
        while (i < _.buf.len) {
            if (_.buf[i] == 0) {
                const start = _.pos;
                _.pos = i + 1;
                return _.buf[start..i];
            }
            i += 1;
        }
        return err.TruncatedPacket;
    }

    fn readLenEncInt(*_): u64! {
        const first = _.readU8()?;
        if (first < 0xFB) {
            return first as u64;
        }
        if (first == 0xFC) {
            return (_.readU16LE()?) as u64;
        }
        if (first == 0xFD) {
            return (_.readU24LE()?) as u64;
        }
        if (first == 0xFE) {
            return _.readU64LE()?;
        }
        return err.InvalidLengthEncodedInt;
    }
}

/// MySQL 客户端命令字节枚举 (COM_*)
pub enum CommandType: u8 {
    COM_QUIT = 0x01,
    COM_INIT_DB = 0x02,
    COM_QUERY = 0x03,
    COM_FIELD_LIST = 0x04,
    COM_CREATE_DB = 0x05,
    COM_DROP_DB = 0x06,
    COM_REFRESH = 0x07,
    COM_SHUTDOWN = 0x08,
    COM_STATISTICS = 0x09,
    COM_PROCESS_INFO = 0x0a,
    COM_CONNECT = 0x0b,
    COM_PROCESS_KILL = 0x0c,
    COM_DEBUG = 0x0d,
    COM_PING = 0x0e,
    COM_TIME = 0x0f,
    COM_DELAYED_INSERT = 0x10,
    COM_CHANGE_USER = 0x11,
    COM_BINLOG_DUMP = 0x12,
    COM_TABLE_DUMP = 0x13,
    COM_CONNECT_OUT = 0x14,
    COM_REGISTER_SLAVE = 0x15,
    COM_STMT_PREPARE = 0x16,
    COM_STMT_EXECUTE = 0x17,
    COM_STMT_SEND_LONG_DATA = 0x18,
    COM_STMT_CLOSE = 0x19,
    COM_STMT_RESET = 0x1a,
    COM_SET_OPTION = 0x1b,
    COM_STMT_FETCH = 0x1c,
    COM_DAEMON = 0x1d,
    COM_BINLOG_DUMP_GTID = 0x1e,
    COM_RESET_CONNECTION = 0x1f,

    static fn fromByte(b: u8): CommandType? {
        if (b >= 0x01 && b <= 0x1f) {
            return b as CommandType;
        }
        return null;
    }

    fn asByte(): u8 {
        return _ as u8;
    }

    fn isPreparedStmt(): bool {
        return _ == .COM_STMT_PREPARE ||
            _ == .COM_STMT_EXECUTE ||
            _ == .COM_STMT_SEND_LONG_DATA ||
            _ == .COM_STMT_CLOSE ||
            _ == .COM_STMT_RESET ||
            _ == .COM_STMT_FETCH;
    }
}

/// MySQL Initial Handshake Packet (Protocol Version 10)
pub struct HandshakeV10 {
    protocol_version: u8 = 10,
    server_version: string = "",
    connection_id: u32 = 0,
    auth_plugin_data_part_1: [8]u8 = [0; 8],
    auth_plugin_data_part_2: []const u8 = "",
    capability_flags: u32 = 0,
    character_set: u8 = 0,
    status_flags: u16 = 0,
    auth_plugin_name: string = "",

    /// 零堆分配解码服务端 HandshakeV10 报文
    static fn decode(buf: []const u8): HandshakeV10! {
        let cur = Cursor{ buf: buf };
        const proto_ver = cur.readU8()?;
        if (proto_ver != 10) {
            return err.UnsupportedProtocolVersion;
        }

        const server_ver = cur.readNulTerminated()?;
        const conn_id = cur.readU32LE()?;

        const part1_slice = cur.readBytes(8)?;
        let part1: [8]u8 = [0; 8];
        @memcpy(part1[0..8], part1_slice);

        // filler byte (0x00)
        * = cur.readU8()?;

        const cap_lower = cur.readU16LE()? as u32;
        const charset = cur.readU8()?;
        const status = cur.readU16LE()?;
        const cap_upper = cur.readU16LE()? as u32;
        const cap_flags: u32 = (cap_upper << 16) | cap_lower;

        let auth_data_len: usize = (cur.readU8()?) as usize;
        if (auth_data_len < 21) {
            auth_data_len = 21;
        }

        // 10 字节保留填充区
        cur.skip(10)?;

        const part2_len = if (auth_data_len > 8) (auth_data_len - 8) else 13;
        const actual_part2_len = if (part2_len < 13) @as(usize, 13) else part2_len;
        const part2 = cur.readBytes(actual_part2_len)?;

        let plugin_name: string = "";
        if ((cap_flags & CAP_CLIENT_PLUGIN_AUTH) != 0 && cur.remaining() > 0) {
            // 部分 MySQL 实现允许末尾无 NUL，优先按 NUL 读取，否则取剩余切片
            plugin_name = cur.readNulTerminated() catch cur.rest();
        }

        return HandshakeV10{
            protocol_version: proto_ver,
            server_version: server_ver,
            connection_id: conn_id,
            auth_plugin_data_part_1: part1,
            auth_plugin_data_part_2: part2,
            capability_flags: cap_flags,
            character_set: charset,
            status_flags: status,
            auth_plugin_name: plugin_name,
        };
    }

    /// 拼接 part_1 (8B) 与 part_2（自动剥离末尾 NUL）为完整认证盐值（标准 20 字节）
    fn writeAuthPluginData(out: []u8): usize {
        let p2_len = _.auth_plugin_data_part_2.len;
        if (p2_len > 0 && _.auth_plugin_data_part_2[p2_len - 1] == 0) {
            p2_len -= 1;
        }
        const total = 8 + p2_len;
        if (out.len < total) {
            return 0;
        }
        @memcpy(out[0..8], _.auth_plugin_data_part_1[0..8]);
        if (p2_len > 0) {
            @memcpy(out[8..total], _.auth_plugin_data_part_2[0..p2_len]);
        }
        return total;
    }

    fn supportsPluginAuth(): bool {
        return (_.capability_flags & CAP_CLIENT_PLUGIN_AUTH) != 0;
    }

    fn supportsSsl(): bool {
        return (_.capability_flags & CAP_CLIENT_SSL) != 0;
    }
}

/// MySQL OK_Packet (0x00 或 CLIENT_DEPRECATE_EOF 模式下的 0xFE)
pub struct OKPacket {
    header: u8 = 0x00,
    affected_rows: u64 = 0,
    last_insert_id: u64 = 0,
    status_flags: u16 = 0,
    warnings: u16 = 0,
    info: string = "",

    static fn decode(buf: []const u8): OKPacket! {
        let cur = Cursor{ buf: buf };
        const hdr = cur.readU8()?;
        if (hdr != 0x00 && hdr != 0xfe) {
            return err.InvalidOKPacket;
        }

        const affected = cur.readLenEncInt()?;
        const insert_id = cur.readLenEncInt()?;
        const status = cur.readU16LE()?;
        const warns = cur.readU16LE()?;
        const info_str: string = cur.rest();

        return OKPacket{
            header: hdr,
            affected_rows: affected,
            last_insert_id: insert_id,
            status_flags: status,
            warnings: warns,
            info: info_str,
        };
    }

    fn isEofStyle(): bool {
        return _.header == 0xfe;
    }
}

/// MySQL ERR_Packet (0xFF)
pub struct ErrorPacket {
    header: u8 = 0xff,
    error_code: u16 = 0,
    has_sql_state: bool = false,
    sql_state: [5]u8 = [0; 5],
    error_message: string = "",

    static fn decode(buf: []const u8): ErrorPacket! {
        let cur = Cursor{ buf: buf };
        const hdr = cur.readU8()?;
        if (hdr != 0xff) {
            return err.InvalidErrorPacket;
        }

        const code = cur.readU16LE()?;
        let has_state = false;
        let state: [5]u8 = [0; 5];

        if (cur.remaining() > 0 && cur.buf[cur.pos] == '#') {
            * = cur.readU8()?;
            const state_slice = cur.readBytes(5)?;
            @memcpy(state[0..5], state_slice);
            has_state = true;
        }

        const msg: string = cur.rest();
        return ErrorPacket{
            header: hdr,
            error_code: code,
            has_sql_state: has_state,
            sql_state: state,
            error_message: msg,
        };
    }

    fn sqlStateSlice(): string? {
        if (!_.has_sql_state) {
            return null;
        }
        return _.sql_state[0..5];
    }
}

/// MySQL EOF_Packet (0xFE, 5 字节)
pub struct EOFPacket {
    header: u8 = 0xfe,
    warnings: u16 = 0,
    status_flags: u16 = 0,

    static fn decode(buf: []const u8): EOFPacket! {
        let cur = Cursor{ buf: buf };
        const hdr = cur.readU8()?;
        if (hdr != 0xfe) {
            return err.InvalidEOFPacket;
        }
        const warns = cur.readU16LE()?;
        const status = cur.readU16LE()?;
        return EOFPacket{
            header: hdr,
            warnings: warns,
            status_flags: status,
        };
    }
}

/// MySQL AuthSwitchRequest Packet (0xFE + plugin_name\0 + plugin_data)
pub struct AuthSwitchRequest {
    header: u8 = 0xfe,
    plugin_name: string = "",
    plugin_data: []const u8 = "",

    static fn decode(buf: []const u8): AuthSwitchRequest! {
        let cur = Cursor{ buf: buf };
        const hdr = cur.readU8()?;
        if (hdr != 0xfe) {
            return err.InvalidAuthSwitchRequest;
        }
        const name = cur.readNulTerminated() catch return err.InvalidAuthSwitchRequest;
        const data = cur.rest();
        return AuthSwitchRequest{
            header: hdr,
            plugin_name: name,
            plugin_data: data,
        };
    }
}
