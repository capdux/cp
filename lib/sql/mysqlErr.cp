// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/ErrorPacket.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation MySQL wire-protocol `ERR_Packet` (0xff) encoder & decoder:
// - Decodes both MySQL 4.1+ (`#` + 5-byte SQLSTATE) and legacy 4.0 ERR packets
// - Zero-copy message slice pointing directly into the input wire buffer
// - Provides SQLSTATE class extraction, retryable deadlock/timeout classification,
//   and wire roundtrip encoder with strict truncation & buffer guards.

/// MySQL ERR_Packet 固定首字节标记（0xff）
pub const MYSQL_ERR_PACKET_HEADER: u8 = 0xff;
/// MySQL 4.1+ SQLSTATE 前导标记字符（'#'）
pub const MYSQL_SQL_STATE_MARKER: u8 = '#';

/// MySQL ERR_Packet 编解码错误集
pub err MySqlErrorPacketError {
    InvalidHeader,
    TruncatedPacket,
    BufferTooSmall,
    InvalidSqlState,
}

/// MySQL 协议 `ERR_Packet` 结构（对齐 Bun `ErrorPacket`）
pub struct MySqlErrorPacket {
    header: u8 = MYSQL_ERR_PACKET_HEADER,
    error_code: u16 = 0,
    has_sql_state: bool = false,
    sql_state: [5]u8 = ['H', 'Y', '0', '0', '0'],
    error_message: []const u8 = "",

    /// 构造 MySQL 4.1+ 带 5 字节 SQLSTATE 的 ERR_Packet
    static fn initWithSqlState(error_code: u16, sql_state: []const u8, message: []const u8): MySqlErrorPacket! {
        if (sql_state.len != 5) {
            return err.InvalidSqlState;
        }
        let state_arr: [5]u8 = [0; 5];
        let i: usize = 0;
        while (i < 5) {
            state_arr[i] = sql_state[i];
            i += 1;
        }
        return MySqlErrorPacket{
            header: MYSQL_ERR_PACKET_HEADER,
            error_code: error_code,
            has_sql_state: true,
            sql_state: state_arr,
            error_message: message,
        };
    }

    /// 构造 MySQL 4.0 兼容（无 SQLSTATE）ERR_Packet
    static fn initLegacy(error_code: u16, message: []const u8): MySqlErrorPacket {
        return MySqlErrorPacket{
            header: MYSQL_ERR_PACKET_HEADER,
            error_code: error_code,
            has_sql_state: false,
            sql_state: ['H', 'Y', '0', '0', '0'],
            error_message: message,
        };
    }

    /// 返回 5 字节 SQLSTATE 切片（若为 4.0 兼容包返回 `null`）
    fn sqlStateSlice(): ([]const u8)? {
        if (!_.has_sql_state) {
            return null;
        }
        return _.sql_state[0..5];
    }

    /// 返回 2 字节 SQLSTATE 大类代码（例如 `"28"` 鉴权、`"42"` 语法、`"23"` 约束、`"HY"` 通用）
    fn sqlStateClass(): ([]const u8)? {
        if (!_.has_sql_state) {
            return null;
        }
        return _.sql_state[0..2];
    }

    /// 判断是否为常见可重试瞬态错误（1040 连接满 / 1205 锁等待超时 / 1213 死锁）
    fn isRetryable(): bool {
        return _.error_code == 1040 || _.error_code == 1205 || _.error_code == 1213;
    }

    /// 计算编码为线协议字节流所需的总字节数
    fn encodedSize(): usize {
        const state_bytes: usize = if (_.has_sql_state) 6 else 0;
        return 3 + state_bytes + _.error_message.len;
    }

    /// 零拷贝解码 MySQL `ERR_Packet` 载荷（对齐 Bun `ErrorPacket.decodeInternal`）
    static fn decode(payload: []const u8): MySqlErrorPacket! {
        if (payload.len < 3) {
            return err.TruncatedPacket;
        }
        if (payload[0] != MYSQL_ERR_PACKET_HEADER) {
            return err.InvalidHeader;
        }
        const code: u16 = @as(u16, payload[1]) | (@as(u16, payload[2]) << 8);
        let pos: usize = 3;
        let has_state = false;
        let state_arr: [5]u8 = ['H', 'Y', '0', '0', '0'];

        if (pos < payload.len && payload[pos] == MYSQL_SQL_STATE_MARKER) {
            if (payload.len < pos + 6) {
                return err.TruncatedPacket;
            }
            has_state = true;
            let i: usize = 0;
            while (i < 5) {
                state_arr[i] = payload[pos + 1 + i];
                i += 1;
            }
            pos += 6;
        }

        return MySqlErrorPacket{
            header: MYSQL_ERR_PACKET_HEADER,
            error_code: code,
            has_sql_state: has_state,
            sql_state: state_arr,
            error_message: payload[pos..],
        };
    }

    /// 将 ERR_Packet 编码写入目标缓冲区，返回写入总字节数
    fn encode(out: []u8): usize! {
        const needed = _.encodedSize();
        if (out.len < needed) {
            return err.BufferTooSmall;
        }
        out[0] = MYSQL_ERR_PACKET_HEADER;
        out[1] = @intCast(_.error_code & 0xff);
        out[2] = @intCast((_.error_code >> 8) & 0xff);
        let pos: usize = 3;
        if (_.has_sql_state) {
            out[pos] = MYSQL_SQL_STATE_MARKER;
            pos += 1;
            let i: usize = 0;
            while (i < 5) {
                out[pos + i] = _.sql_state[i];
                i += 1;
            }
            pos += 5;
        }
        let k: usize = 0;
        while (k < _.error_message.len) {
            out[pos + k] = _.error_message[k];
            k += 1;
        }
        return needed;
    }
}
