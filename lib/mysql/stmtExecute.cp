// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/PreparedStatement.cp | Synced: 2026-11-23T01:34:29Z
// Pure-CP zero-allocation MySQL Binary Protocol `COM_STMT_EXECUTE` (0x17) request codec.
// Implements cursor flags, NULL-bitmap encoding/decoding, new_params_bind_flag, typed binary
// and length-encoded parameter serialization, and zero-copy frame header decoding.

/// MySQL `COM_STMT_EXECUTE` 命令字节常量
pub const STMT_EXECUTE_CMD: u8 = 0x17;

/// `COM_STMT_EXECUTE` 固定头部长度（1 cmd + 4 statement_id + 1 flags + 4 iteration_count）
pub const STMT_EXECUTE_HEADER_LEN: usize = 10;

/// 默认 `iteration_count`（协议规范固定为 1）
pub const STMT_EXECUTE_DEFAULT_ITERATION_COUNT: u32 = 1;

/// 零堆分配最大绑定参数个数
pub const STMT_EXECUTE_MAX_PARAMS: usize = 64;

/// 参数类型无符号标志位（写入 parameter type 的第 2 字节）
pub const STMT_PARAM_UNSIGNED_FLAG: u8 = 0x80;

/// `new_params_bind_flag` 字节常量
pub const STMT_NEW_PARAMS_BIND_NONE: u8 = 0x00;
pub const STMT_NEW_PARAMS_BIND_YES: u8 = 0x01;

/// `COM_STMT_EXECUTE` 游标与扩展标志位常量
pub const STMT_CURSOR_TYPE_NO_CURSOR: u8 = 0x00;
pub const STMT_CURSOR_TYPE_READ_ONLY: u8 = 0x01;
pub const STMT_CURSOR_TYPE_FOR_UPDATE: u8 = 0x02;
pub const STMT_CURSOR_TYPE_SCROLLABLE: u8 = 0x04;
pub const STMT_PARAMETER_COUNT_AVAILABLE: u8 = 0x08;

/// 常用 MySQL 二进制协议字段类型字节常量
pub const STMT_FTYPE_TINY: u8 = 0x01;
pub const STMT_FTYPE_SHORT: u8 = 0x02;
pub const STMT_FTYPE_LONG: u8 = 0x03;
pub const STMT_FTYPE_FLOAT: u8 = 0x04;
pub const STMT_FTYPE_DOUBLE: u8 = 0x05;
pub const STMT_FTYPE_NULL: u8 = 0x06;
pub const STMT_FTYPE_LONGLONG: u8 = 0x08;
pub const STMT_FTYPE_BLOB: u8 = 0xFC;
pub const STMT_FTYPE_VAR_STRING: u8 = 0xFD;

/// 单包最大载荷长度（24-bit `0xFFFFFF`）
pub const STMT_EXECUTE_MAX_PAYLOAD: usize = 0xFFFFFF;

/// `COM_STMT_EXECUTE` 编解码与边界校验错误集
pub err StmtExecuteError {
    TooManyParams,
    BufferTooSmall,
    Truncated,
    InvalidCommand,
    InvalidBindFlag,
    InvalidParamPayload,
    PayloadTooLarge,
    ParamIndexOutOfBounds,
}

/// `COM_STMT_EXECUTE` 游标模式枚举
pub enum StmtCursorType {
    NoCursor,
    ReadOnly,
    ForUpdate,
    Scrollable,

    /// 转换为协议标志位字节
    fn toByte(): u8 {
        if (_ == StmtCursorType.ReadOnly) return STMT_CURSOR_TYPE_READ_ONLY;
        if (_ == StmtCursorType.ForUpdate) return STMT_CURSOR_TYPE_FOR_UPDATE;
        if (_ == StmtCursorType.Scrollable) return STMT_CURSOR_TYPE_SCROLLABLE;
        return STMT_CURSOR_TYPE_NO_CURSOR;
    }

    /// 从 `flags` 字节提取低 3 位游标模式
    static fn fromFlags(flags: u8): StmtCursorType {
        const low = flags & 0x07;
        if (low == STMT_CURSOR_TYPE_READ_ONLY) return StmtCursorType.ReadOnly;
        if (low == STMT_CURSOR_TYPE_FOR_UPDATE) return StmtCursorType.ForUpdate;
        if (low == STMT_CURSOR_TYPE_SCROLLABLE) return StmtCursorType.Scrollable;
        return StmtCursorType.NoCursor;
    }
}

/// 判断给定 MySQL 字段类型在 `COM_STMT_EXECUTE` 中是否采用定长二进制编码（无需 length-encoded 前缀）
pub fn isStmtBinaryFixedType(field_type: u8): bool {
    return field_type == STMT_FTYPE_TINY ||
        field_type == STMT_FTYPE_SHORT ||
        field_type == STMT_FTYPE_LONG ||
        field_type == STMT_FTYPE_FLOAT ||
        field_type == STMT_FTYPE_DOUBLE ||
        field_type == STMT_FTYPE_LONGLONG;
}

/// 计算 `param_count` 个参数所需的 NULL-bitmap 字节数（`(n + 7) / 8`）
pub fn stmtNullBitmapLen(param_count: usize): usize {
    if (param_count == 0) {
        return 0;
    }
    return (param_count + 7) / 8;
}

/// 查询 NULL-bitmap 中第 `index`（0-based）个参数是否为 SQL `NULL`
pub fn isStmtNullInBitmap(null_bitmap: []const u8, index: usize): bool {
    const byte_idx = index >> 3;
    if (byte_idx >= null_bitmap.len) {
        return false;
    }
    const bit: u3 = @as(u3, @intCast(index & 7));
    return ((null_bitmap[byte_idx] >> bit) & 1) != 0;
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

fn readLeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn writeLeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast(v & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 3] = @as(u8, @intCast((v >> 24) & 0xFF));
}

fn writeFrameHeader(out: []u8, payload_len: usize, seq_id: u8) {
    out[0] = @as(u8, @intCast(payload_len & 0xFF));
    out[1] = @as(u8, @intCast((payload_len >> 8) & 0xFF));
    out[2] = @as(u8, @intCast((payload_len >> 16) & 0xFF));
    out[3] = seq_id;
}

fn readFramePayloadLen(frame: []const u8): usize {
    return @as(usize, frame[0]) |
        (@as(usize, frame[1]) << 8) |
        (@as(usize, frame[2]) << 16);
}

fn copySlice(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

/// 零堆分配 `COM_STMT_EXECUTE` 单参数描述符（内置 8 字节标量小端缓存，避免临时值悬空）
pub struct StmtExecParam {
    field_type: u8 = STMT_FTYPE_NULL,
    is_unsigned: bool = false,
    is_null: bool = true,
    is_binary_fixed: bool = false,
    use_inline: bool = false,
    inline_len: u8 = 0,
    inline_buf: [8]u8 = [0; 8],
    ext_data: []const u8 = "",

    /// 构造 SQL `NULL` 参数
    static fn nullParam(): StmtExecParam {
        return StmtExecParam{};
    }

    /// 构造带符号 8 位整数参数（`MYSQL_TYPE_TINY`）
    static fn int8(v: i8): StmtExecParam {
        let p = StmtExecParam{
            field_type: STMT_FTYPE_TINY,
            is_unsigned: false,
            is_null: false,
            is_binary_fixed: true,
            use_inline: true,
            inline_len: 1,
        };
        const u: u8 = @bitCast(v);
        p.inline_buf[0] = u;
        return p;
    }

    /// 构造无符号 8 位整数参数（`MYSQL_TYPE_TINY | UNSIGNED`）
    static fn uint8(v: u8): StmtExecParam {
        let p = StmtExecParam{
            field_type: STMT_FTYPE_TINY,
            is_unsigned: true,
            is_null: false,
            is_binary_fixed: true,
            use_inline: true,
            inline_len: 1,
        };
        p.inline_buf[0] = v;
        return p;
    }

    /// 构造带符号 32 位小端整数参数（`MYSQL_TYPE_LONG`）
    static fn int32(v: i32): StmtExecParam {
        let p = StmtExecParam{
            field_type: STMT_FTYPE_LONG,
            is_unsigned: false,
            is_null: false,
            is_binary_fixed: true,
            use_inline: true,
            inline_len: 4,
        };
        const u: u32 = @bitCast(v);
        p.inline_buf[0] = @as(u8, @intCast(u & 0xFF));
        p.inline_buf[1] = @as(u8, @intCast((u >> 8) & 0xFF));
        p.inline_buf[2] = @as(u8, @intCast((u >> 16) & 0xFF));
        p.inline_buf[3] = @as(u8, @intCast((u >> 24) & 0xFF));
        return p;
    }

    /// 构造无符号 32 位小端整数参数（`MYSQL_TYPE_LONG | UNSIGNED`）
    static fn uint32(v: u32): StmtExecParam {
        let p = StmtExecParam{
            field_type: STMT_FTYPE_LONG,
            is_unsigned: true,
            is_null: false,
            is_binary_fixed: true,
            use_inline: true,
            inline_len: 4,
        };
        p.inline_buf[0] = @as(u8, @intCast(v & 0xFF));
        p.inline_buf[1] = @as(u8, @intCast((v >> 8) & 0xFF));
        p.inline_buf[2] = @as(u8, @intCast((v >> 16) & 0xFF));
        p.inline_buf[3] = @as(u8, @intCast((v >> 24) & 0xFF));
        return p;
    }

    /// 构造带符号 64 位小端整数参数（`MYSQL_TYPE_LONGLONG`）
    static fn int64(v: i64): StmtExecParam {
        let p = StmtExecParam{
            field_type: STMT_FTYPE_LONGLONG,
            is_unsigned: false,
            is_null: false,
            is_binary_fixed: true,
            use_inline: true,
            inline_len: 8,
        };
        const u: u64 = @bitCast(v);
        let i: usize = 0;
        while (i < 8) {
            p.inline_buf[i] = @as(u8, @intCast((u >> @as(u6, @intCast(i * 8))) & 0xFF));
            i += 1;
        }
        return p;
    }

    /// 构造无符号 64 位小端整数参数（`MYSQL_TYPE_LONGLONG | UNSIGNED`）
    static fn uint64(v: u64): StmtExecParam {
        let p = StmtExecParam{
            field_type: STMT_FTYPE_LONGLONG,
            is_unsigned: true,
            is_null: false,
            is_binary_fixed: true,
            use_inline: true,
            inline_len: 8,
        };
        let i: usize = 0;
        while (i < 8) {
            p.inline_buf[i] = @as(u8, @intCast((v >> @as(u6, @intCast(i * 8))) & 0xFF));
            i += 1;
        }
        return p;
    }

    /// 构造 IEEE-754 64 位小端浮点参数（`MYSQL_TYPE_DOUBLE`）
    static fn float64(v: f64): StmtExecParam {
        let p = StmtExecParam{
            field_type: STMT_FTYPE_DOUBLE,
            is_unsigned: false,
            is_null: false,
            is_binary_fixed: true,
            use_inline: true,
            inline_len: 8,
        };
        const u: u64 = @bitCast(v);
        let i: usize = 0;
        while (i < 8) {
            p.inline_buf[i] = @as(u8, @intCast((u >> @as(u6, @intCast(i * 8))) & 0xFF));
            i += 1;
        }
        return p;
    }

    /// 构造 UTF-8 变长字符串参数（`MYSQL_TYPE_VAR_STRING`，采用 `lenenc-str` 编码）
    static fn varString(s: []const u8): StmtExecParam {
        return StmtExecParam{
            field_type: STMT_FTYPE_VAR_STRING,
            is_unsigned: false,
            is_null: false,
            is_binary_fixed: false,
            use_inline: false,
            inline_len: 0,
            ext_data: s,
        };
    }

    /// 构造二进制 BLOB 参数（`MYSQL_TYPE_BLOB`，采用 `lenenc-str` 编码）
    static fn blob(b: []const u8): StmtExecParam {
        return StmtExecParam{
            field_type: STMT_FTYPE_BLOB,
            is_unsigned: false,
            is_null: false,
            is_binary_fixed: false,
            use_inline: false,
            inline_len: 0,
            ext_data: b,
        };
    }

    /// 构造自定义定长二进制参数（直接写入 `raw` 字节，无 `lenenc` 长度前缀）
    static fn fixedBinary(field_type: u8, is_unsigned: bool, raw: []const u8): StmtExecParam {
        return StmtExecParam{
            field_type: field_type,
            is_unsigned: is_unsigned,
            is_null: false,
            is_binary_fixed: true,
            use_inline: false,
            inline_len: 0,
            ext_data: raw,
        };
    }

    /// 返回参数原始值字节长度（未包含 `lenenc` 前缀）
    fn rawDataLen(): usize {
        if (_.is_null || _.field_type == STMT_FTYPE_NULL) {
            return 0;
        }
        if (_.use_inline) {
            return @as(usize, _.inline_len);
        }
        return _.ext_data.len;
    }

    /// 返回参数在 `COM_STMT_EXECUTE` 值区的实际序列化字节数
    fn encodedValueLen(): usize {
        if (_.is_null || _.field_type == STMT_FTYPE_NULL) {
            return 0;
        }
        const n = _.rawDataLen();
        if (_.is_binary_fixed) {
            return n;
        }
        return lenEncSize(@as(u64, n)) + n;
    }

    /// 将单个参数值写入输出缓冲区，返回写入字节数
    fn writeValueTo(out: []u8, off: usize): usize {
        if (_.is_null || _.field_type == STMT_FTYPE_NULL) {
            return 0;
        }
        let pos = off;
        const n = _.rawDataLen();
        if (!_.is_binary_fixed) {
            pos += writeLenEnc(out, pos, @as(u64, n));
        }
        if (_.use_inline) {
            let i: usize = 0;
            while (i < n) {
                out[pos + i] = _.inline_buf[i];
                i += 1;
            }
        } else {
            copySlice(out, pos, _.ext_data);
        }
        return (pos + n) - off;
    }
}

/// 解码后的 `COM_STMT_EXECUTE` 请求头与零拷贝 NULL-bitmap 视图
pub struct StmtExecuteHeader {
    statement_id: u32 = 0,
    flags: u8 = 0,
    iteration_count: u32 = STMT_EXECUTE_DEFAULT_ITERATION_COUNT,
    sequence_id: u8 = 0,
    param_count: usize = 0,
    null_bitmap: []const u8 = "",
    new_params_bind_flag: bool = false,
    params_offset: usize = STMT_EXECUTE_HEADER_LEN,

    /// 查询第 `index`（0-based）个参数在 NULL-bitmap 中是否为 `NULL`
    fn isParamNull(index: usize): bool {
        if (index >= _.param_count) {
            return false;
        }
        return isStmtNullInBitmap(_.null_bitmap, index);
    }

    /// 提取游标类型枚举
    fn cursorType(): StmtCursorType {
        return StmtCursorType.fromFlags(_.flags);
    }

    /// 判断是否置位 `PARAMETER_COUNT_AVAILABLE`（0x08）
    fn hasParamCountAvailable(): bool {
        return (_.flags & STMT_PARAMETER_COUNT_AVAILABLE) != 0;
    }
}

/// 零堆分配 MySQL `COM_STMT_EXECUTE`（0x17）请求帧编解码器
pub struct StmtExecutePacket {
    statement_id: u32 = 0,
    flags: u8 = STMT_CURSOR_TYPE_NO_CURSOR,
    iteration_count: u32 = STMT_EXECUTE_DEFAULT_ITERATION_COUNT,
    new_params_bind_flag: bool = true,
    param_count: usize = 0,
    params: [64]StmtExecParam = [StmtExecParam{}; 64],

    /// 构造默认 `COM_STMT_EXECUTE` 请求（无游标、`iteration_count = 1`、`new_params_bind_flag = true`）
    static fn init(statement_id: u32): StmtExecutePacket {
        return StmtExecutePacket{
            statement_id: statement_id,
            flags: STMT_CURSOR_TYPE_NO_CURSOR,
            iteration_count: STMT_EXECUTE_DEFAULT_ITERATION_COUNT,
            new_params_bind_flag: true,
            param_count: 0,
        };
    }

    /// 构造带显式游标标志与绑定标志的 `COM_STMT_EXECUTE` 请求
    static fn initWithFlags(
        statement_id: u32,
        flags: u8,
        new_params_bind_flag: bool,
    ): StmtExecutePacket {
        return StmtExecutePacket{
            statement_id: statement_id,
            flags: flags,
            iteration_count: STMT_EXECUTE_DEFAULT_ITERATION_COUNT,
            new_params_bind_flag: new_params_bind_flag,
            param_count: 0,
        };
    }

    /// 追加一个绑定参数，返回追加后的参数总数
    fn addParam(*_, p: StmtExecParam): StmtExecuteError!usize {
        if (_.param_count >= STMT_EXECUTE_MAX_PARAMS) {
            return StmtExecuteError.TooManyParams;
        }
        _.params[_.param_count] = p;
        _.param_count += 1;
        return _.param_count;
    }

    /// 计算当前参数集合的 NULL-bitmap 字节长度
    fn nullBitmapLen(): usize {
        return stmtNullBitmapLen(_.param_count);
    }

    /// 将当前参数的 NULL-bitmap 写入 `out` 缓冲区，返回写入字节数
    fn writeNullBitmap(out: []u8): StmtExecuteError!usize {
        const bitmap_len = _.nullBitmapLen();
        if (bitmap_len == 0) {
            return 0;
        }
        if (out.len < bitmap_len) {
            return StmtExecuteError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < bitmap_len) {
            out[i] = 0;
            i += 1;
        }
        let idx: usize = 0;
        while (idx < _.param_count) {
            if (_.params[idx].is_null || _.params[idx].field_type == STMT_FTYPE_NULL) {
                const bit: u3 = @as(u3, @intCast(idx & 7));
                out[idx >> 3] |= (@as(u8, 1) << bit);
            }
            idx += 1;
        }
        return bitmap_len;
    }

    /// 计算不含 4 字节 MySQL 帧头的载荷总字节数
    fn payloadLen(): usize {
        let total: usize = STMT_EXECUTE_HEADER_LEN;
        if (_.param_count > 0) {
            total += _.nullBitmapLen();
            total += 1; // new_params_bind_flag
            if (_.new_params_bind_flag) {
                total += _.param_count * 2;
            }
            let i: usize = 0;
            while (i < _.param_count) {
                total += _.params[i].encodedValueLen();
                i += 1;
            }
        }
        return total;
    }

    /// 计算含 4 字节 MySQL 帧头的完整帧字节数
    fn frameLen(): usize {
        return 4 + _.payloadLen();
    }

    /// 编码 `COM_STMT_EXECUTE` 原始载荷（不含 4 字节帧头）到 `out`，返回写入字节数
    fn encodePayload(out: []u8): StmtExecuteError!usize {
        const needed = _.payloadLen();
        if (needed > STMT_EXECUTE_MAX_PAYLOAD) {
            return StmtExecuteError.PayloadTooLarge;
        }
        if (out.len < needed) {
            return StmtExecuteError.BufferTooSmall;
        }

        out[0] = STMT_EXECUTE_CMD;
        writeLeU32(out, 1, _.statement_id);
        out[5] = _.flags;
        writeLeU32(out, 6, _.iteration_count);

        let pos: usize = STMT_EXECUTE_HEADER_LEN;
        if (_.param_count > 0) {
            const bm_written = _.writeNullBitmap(out[pos..])?;
            pos += bm_written;

            out[pos] = if (_.new_params_bind_flag) STMT_NEW_PARAMS_BIND_YES else STMT_NEW_PARAMS_BIND_NONE;
            pos += 1;

            if (_.new_params_bind_flag) {
                let i: usize = 0;
                while (i < _.param_count) {
                    out[pos] = _.params[i].field_type;
                    out[pos + 1] = if (_.params[i].is_unsigned) STMT_PARAM_UNSIGNED_FLAG else 0;
                    pos += 2;
                    i += 1;
                }
            }

            let idx: usize = 0;
            while (idx < _.param_count) {
                pos += _.params[idx].writeValueTo(out, pos);
                idx += 1;
            }
        }
        return pos;
    }

    /// 编码带 4 字节 MySQL 帧头的完整 `COM_STMT_EXECUTE` 帧到 `out`，返回写入字节数
    fn encodeFrame(sequence_id: u8, out: []u8): StmtExecuteError!usize {
        const plen = _.payloadLen();
        if (plen > STMT_EXECUTE_MAX_PAYLOAD) {
            return StmtExecuteError.PayloadTooLarge;
        }
        if (out.len < 4 + plen) {
            return StmtExecuteError.BufferTooSmall;
        }
        writeFrameHeader(out, plen, sequence_id);
        const written = _.encodePayload(out[4..])?;
        return 4 + written;
    }

    /// 解码 `COM_STMT_EXECUTE` 原始载荷头部与 NULL-bitmap（不含 4 字节帧头）
    static fn decodePayloadHeader(payload: []const u8, param_count: usize): StmtExecuteError!StmtExecuteHeader {
        if (payload.len < STMT_EXECUTE_HEADER_LEN) {
            return StmtExecuteError.Truncated;
        }
        if (payload[0] != STMT_EXECUTE_CMD) {
            return StmtExecuteError.InvalidCommand;
        }
        const stmt_id = readLeU32(payload, 1);
        const flags = payload[5];
        const iter_count = readLeU32(payload, 6);

        if (param_count == 0) {
            return StmtExecuteHeader{
                statement_id: stmt_id,
                flags: flags,
                iteration_count: iter_count,
                sequence_id: 0,
                param_count: 0,
                null_bitmap: payload[STMT_EXECUTE_HEADER_LEN..STMT_EXECUTE_HEADER_LEN],
                new_params_bind_flag: false,
                params_offset: STMT_EXECUTE_HEADER_LEN,
            };
        }

        const bm_len = stmtNullBitmapLen(param_count);
        const bind_flag_pos = STMT_EXECUTE_HEADER_LEN + bm_len;
        if (payload.len < bind_flag_pos + 1) {
            return StmtExecuteError.Truncated;
        }
        const bind_byte = payload[bind_flag_pos];
        if (bind_byte != STMT_NEW_PARAMS_BIND_NONE && bind_byte != STMT_NEW_PARAMS_BIND_YES) {
            return StmtExecuteError.InvalidBindFlag;
        }
        const new_bind = bind_byte == STMT_NEW_PARAMS_BIND_YES;
        const params_off = bind_flag_pos + 1;
        if (new_bind && payload.len < params_off + param_count * 2) {
            return StmtExecuteError.Truncated;
        }

        return StmtExecuteHeader{
            statement_id: stmt_id,
            flags: flags,
            iteration_count: iter_count,
            sequence_id: 0,
            param_count: param_count,
            null_bitmap: payload[STMT_EXECUTE_HEADER_LEN..bind_flag_pos],
            new_params_bind_flag: new_bind,
            params_offset: params_off,
        };
    }

    /// 解码带 4 字节 MySQL 帧头的完整 `COM_STMT_EXECUTE` 帧头部
    static fn decodeFrameHeader(frame: []const u8, param_count: usize): StmtExecuteError!StmtExecuteHeader {
        if (frame.len < 4) {
            return StmtExecuteError.Truncated;
        }
        const plen = readFramePayloadLen(frame);
        if (frame.len < 4 + plen) {
            return StmtExecuteError.Truncated;
        }
        let hdr = StmtExecutePacket.decodePayloadHeader(frame[4..(4 + plen)], param_count)?;
        hdr.sequence_id = frame[3];
        return hdr;
    }
}
