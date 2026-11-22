// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/Query.cp | Synced: 2026-11-22T20:52:20Z
// Pure-CP zero-allocation MySQL `COM_QUERY` (simple & parameterized with NULL-bitmap / named attributes)
// and single-byte session command (`COM_PING` / `COM_QUIT` / `COM_INIT_DB`) wire frame codec.

/// MySQL 客户端命令字节常量
pub const MYSQL_CMD_QUIT: u8 = 0x01;
pub const MYSQL_CMD_INIT_DB: u8 = 0x02;
pub const MYSQL_CMD_QUERY: u8 = 0x03;
pub const MYSQL_CMD_PING: u8 = 0x0E;

/// 常用 MySQL 字段类型字节常量（用于 `MySqlQueryParam`）
pub const MYSQL_FTYPE_LONG: u8 = 0x03;
pub const MYSQL_FTYPE_DOUBLE: u8 = 0x05;
pub const MYSQL_FTYPE_NULL: u8 = 0x06;
pub const MYSQL_FTYPE_LONGLONG: u8 = 0x08;
pub const MYSQL_FTYPE_VAR_STRING: u8 = 0xFD;

/// `MySqlQueryPacket` 零堆分配最大绑定参数数量
pub const MYSQL_QUERY_MAX_PARAMS: usize = 32;

/// 参数类型无符号标志位与新参数绑定标志字节
pub const MYSQL_PARAM_UNSIGNED_FLAG: u8 = 0x80;
pub const MYSQL_NEW_PARAMS_BOUND_FLAG: u8 = 0x01;

/// 单包最大载荷字节数（24-bit `0xFFFFFF`）
pub const MYSQL_QUERY_MAX_PAYLOAD: usize = 0xFFFFFF;

/// `MySqlQueryPacket` 编解码与边界校验错误集
pub err QueryPacketError {
    EmptyQuery,
    TooManyParameters,
    InvalidCommand,
    Truncated,
    TrailingBytes,
    BufferTooSmall,
    PayloadTooLarge,
}

/// 计算 `param_count` 个参数所需的 MySQL NULL-bitmap 字节长度（`(n + 7) / 8`）
pub fn mysqlNullBitmapLen(param_count: usize): usize {
    if (param_count == 0) {
        return 0;
    }
    return (param_count + 7) / 8;
}

/// 读取 MySQL NULL-bitmap 中第 `idx` 位（0-based）是否为 SQL `NULL`
pub fn isNullInBitmap(bitmap: []const u8, idx: usize): bool {
    const byte_idx = idx / 8;
    if (byte_idx >= bitmap.len) {
        return false;
    }
    const bit: u3 = @as(u3, @intCast(idx % 8));
    return ((bitmap[byte_idx] >> bit) & 1) != 0;
}

fn lenEncSize(v: u64): usize {
    if (v < 251) return 1;
    if (v <= 0xFFFF) return 3;
    if (v <= 0xFFFFFF) return 4;
    return 9;
}

fn writeLenEnc(out: []u8, off: usize, v: u64): usize {
    if (v < 251) {
        out[off] = @as(u8, @intCast(v));
        return 1;
    }
    if (v <= 0xFFFF) {
        out[off] = 0xFC;
        out[off + 1] = @as(u8, @intCast(v & 0xFF));
        out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
        return 3;
    }
    if (v <= 0xFFFFFF) {
        out[off] = 0xFD;
        out[off + 1] = @as(u8, @intCast(v & 0xFF));
        out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
        out[off + 3] = @as(u8, @intCast((v >> 16) & 0xFF));
        return 4;
    }
    out[off] = 0xFE;
    let i: usize = 0;
    while (i < 8) {
        out[off + 1 + i] = @as(u8, @intCast((v >> @as(u6, @intCast(i * 8))) & 0xFF));
        i += 1;
    }
    return 9;
}

fn writePacketHeader(out: []u8, payload_len: usize, seq_id: u8) {
    out[0] = @as(u8, @intCast(payload_len & 0xFF));
    out[1] = @as(u8, @intCast((payload_len >> 8) & 0xFF));
    out[2] = @as(u8, @intCast((payload_len >> 16) & 0xFF));
    out[3] = seq_id;
}

fn readPacketPayloadLen(frame: []const u8): usize {
    return @as(usize, frame[0]) |
        (@as(usize, frame[1]) << 8) |
        (@as(usize, frame[2]) << 16);
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

/// 格式化 1-based 参数属性名 `":p{idx}"`（如 `:p1`..`:p32`）写入栈缓冲并返回长度
fn formatParamName(idx_1based: usize, dst: []u8): usize {
    dst[0] = ':';
    dst[1] = 'p';
    if (idx_1based < 10) {
        dst[2] = @as(u8, @intCast('0' + idx_1based));
        return 3;
    }
    const tens = idx_1based / 10;
    const ones = idx_1based % 10;
    dst[2] = @as(u8, @intCast('0' + tens));
    dst[3] = @as(u8, @intCast('0' + ones));
    return 4;
}

fn paramNameLen(idx_1based: usize): usize {
    if (idx_1based < 10) return 3;
    return 4;
}

/// 单条 MySQL 参数化查询绑定参数（零堆分配）
pub struct MySqlQueryParam {
    field_type: u8 = MYSQL_FTYPE_NULL,
    is_unsigned: bool = false,
    is_binary_fixed: bool = false,
    value: ([]const u8)? = null,

    static fn nullParam(): MySqlQueryParam {
        return MySqlQueryParam{
            field_type: MYSQL_FTYPE_NULL,
            is_unsigned: false,
            is_binary_fixed: false,
            value: null,
        };
    }

    static fn varString(s: []const u8): MySqlQueryParam {
        return MySqlQueryParam{
            field_type: MYSQL_FTYPE_VAR_STRING,
            is_unsigned: false,
            is_binary_fixed: false,
            value: s,
        };
    }

    static fn fixedBinary(ftype: u8, unsigned_flag: bool, raw: []const u8): MySqlQueryParam {
        return MySqlQueryParam{
            field_type: ftype,
            is_unsigned: unsigned_flag,
            is_binary_fixed: true,
            value: raw,
        };
    }

    fn isNull(): bool {
        return _.value == null || _.field_type == MYSQL_FTYPE_NULL;
    }
}

/// MySQL `COM_QUERY`（`0x03`）零堆分配帧编码器与解码器
///
/// 对齐 Bun `Query.cp`：
/// - 无参数时编码为 `[4B header][0x03][sql]`；
/// - 携带参数时编码为 `[4B header][0x03][sql][null_bitmap][0x01]([type][0x80|0x00][lenenc ":p{i}"])*([fixed|lenenc_str])*`。
pub struct MySqlQueryPacket {
    sql: []const u8 = "",
    params: [MYSQL_QUERY_MAX_PARAMS]MySqlQueryParam = [MySqlQueryParam{}; MYSQL_QUERY_MAX_PARAMS],
    param_count: usize = 0,

    static fn init(sql: []const u8): MySqlQueryPacket {
        return MySqlQueryPacket{
            sql: sql,
        };
    }

    fn addParam(*_, param: MySqlQueryParam): usize! {
        if (_.param_count >= MYSQL_QUERY_MAX_PARAMS) {
            return err.TooManyParameters;
        }
        const idx = _.param_count;
        _.params[idx] = param;
        _.param_count += 1;
        return idx;
    }

    fn paramCount(): usize {
        return _.param_count;
    }

    fn paramAt(idx: usize): MySqlQueryParam? {
        if (idx >= _.param_count) {
            return null;
        }
        return _.params[idx];
    }

    fn nullBitmapLen(): usize {
        return mysqlNullBitmapLen(_.param_count);
    }

    fn encodedPayloadLen(): usize {
        let total: usize = _.sql.len + 1;
        if (_.param_count == 0) {
            return total;
        }
        total += mysqlNullBitmapLen(_.param_count);
        total += 1; // new_params_bind_flag = 0x01

        let i: usize = 0;
        while (i < _.param_count) {
            const name_len = paramNameLen(i + 1);
            total += 2 + lenEncSize(@as(u64, @intCast(name_len))) + name_len;
            i += 1;
        }

        i = 0;
        while (i < _.param_count) {
            const p = _.params[i];
            if (!p.isNull()) {
                if (v := p.value) {
                    if (p.is_binary_fixed) {
                        total += v.len;
                    } else {
                        total += lenEncSize(@as(u64, @intCast(v.len))) + v.len;
                    }
                }
            }
            i += 1;
        }
        return total;
    }

    fn encodedFrameLen(): usize {
        return 4 + _.encodedPayloadLen();
    }

    /// 编码为完整 MySQL `COM_QUERY` 线协议帧（零堆分配）
    fn encodeFrame(sequence_id: u8, out: []u8): usize! {
        if (_.sql.len == 0) {
            return err.EmptyQuery;
        }
        const payload_len = _.encodedPayloadLen();
        if (payload_len > MYSQL_QUERY_MAX_PAYLOAD) {
            return err.PayloadTooLarge;
        }
        const total = 4 + payload_len;
        if (out.len < total) {
            return err.BufferTooSmall;
        }

        writePacketHeader(out, payload_len, sequence_id);
        let pos: usize = 4;
        out[pos] = MYSQL_CMD_QUERY;
        pos += 1;
        copyBytes(out, pos, _.sql);
        pos += _.sql.len;

        if (_.param_count == 0) {
            return pos;
        }

        // 1. Write NULL-bitmap
        const bm_len = mysqlNullBitmapLen(_.param_count);
        let b: usize = 0;
        while (b < bm_len) {
            out[pos + b] = 0;
            b += 1;
        }
        let i: usize = 0;
        while (i < _.param_count) {
            if (_.params[i].isNull()) {
                const bit: u3 = @as(u3, @intCast(i % 8));
                out[pos + (i / 8)] |= (@as(u8, 1) << bit);
            }
            i += 1;
        }
        pos += bm_len;

        // 2. Write new_params_bound_flag + type metadata + named attribute ":p{i}"
        out[pos] = MYSQL_NEW_PARAMS_BOUND_FLAG;
        pos += 1;

        let name_buf: [8]u8 = [0; 8];
        i = 0;
        while (i < _.param_count) {
            const p = _.params[i];
            out[pos] = p.field_type;
            out[pos + 1] = if (p.is_unsigned) MYSQL_PARAM_UNSIGNED_FLAG else 0;
            pos += 2;
            const nlen = formatParamName(i + 1, name_buf[0..]);
            pos += writeLenEnc(out, pos, @as(u64, @intCast(nlen)));
            copyBytes(out, pos, name_buf[0..nlen]);
            pos += nlen;
            i += 1;
        }

        // 3. Write non-NULL parameter values
        i = 0;
        while (i < _.param_count) {
            const p = _.params[i];
            if (!p.isNull()) {
                if (v := p.value) {
                    if (p.is_binary_fixed) {
                        copyBytes(out, pos, v);
                        pos += v.len;
                    } else {
                        pos += writeLenEnc(out, pos, @as(u64, @intCast(v.len)));
                        copyBytes(out, pos, v);
                        pos += v.len;
                    }
                }
            }
            i += 1;
        }

        return pos;
    }

    /// 零拷贝解码无参 `COM_QUERY` 帧，返回 SQL 字符串切片
    static fn decodeSimpleFrame(frame: []const u8): ([]const u8)! {
        if (frame.len < 5) {
            return err.Truncated;
        }
        const payload_len = readPacketPayloadLen(frame);
        const expected = 4 + payload_len;
        if (frame.len < expected) {
            return err.Truncated;
        }
        if (frame.len > expected) {
            return err.TrailingBytes;
        }
        if (payload_len < 1 || frame[4] != MYSQL_CMD_QUERY) {
            return err.InvalidCommand;
        }
        const sql_slice = frame[5..expected];
        if (sql_slice.len == 0) {
            return err.EmptyQuery;
        }
        return sql_slice;
    }

    /// 编码 `COM_INIT_DB`（`0x02`）切换默认数据库帧
    static fn encodeInitDbFrame(schema_name: []const u8, sequence_id: u8, out: []u8): usize! {
        if (schema_name.len == 0) {
            return err.EmptyQuery;
        }
        const payload_len = 1 + schema_name.len;
        if (payload_len > MYSQL_QUERY_MAX_PAYLOAD) {
            return err.PayloadTooLarge;
        }
        const total = 4 + payload_len;
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        writePacketHeader(out, payload_len, sequence_id);
        out[4] = MYSQL_CMD_INIT_DB;
        copyBytes(out, 5, schema_name);
        return total;
    }

    /// 编码 5 字节无载荷单命令帧（如 `MYSQL_CMD_PING` / `MYSQL_CMD_QUIT`）
    static fn encodeSingleCmdFrame(cmd: u8, sequence_id: u8, out: []u8): usize! {
        if (out.len < 5) {
            return err.BufferTooSmall;
        }
        writePacketHeader(out, 1, sequence_id);
        out[4] = cmd;
        return 5;
    }
}
