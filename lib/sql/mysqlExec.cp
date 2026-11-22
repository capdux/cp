// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/PreparedStatement.cp | Synced: 2026-11-22T10:45:27Z
// Ported from Bun: /data/dev/bun/src/sql/mysql/protocol/PreparedStatement.cp & /data/dev/bun/docs/runtime/sql.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation MySQL 4.1/8.0 `COM_STMT_EXECUTE` (0x17) binary protocol encoder & decoder:
// NULL bitmap packing/unpacking, `new_params_bind_flag` type+unsigned header encoding,
// little-endian fixed-width binary values (TINY/SHORT/LONG/LONGLONG/FLOAT/DOUBLE),
// length-encoded string/blob/JSON/DECIMAL parameters, and framed packet round-trip codec.

import {
    MYSQL_PACKET_HEADER_SIZE,
    MySqlWireWriter,
} from "./mysqlWriter.cp";
import { MySqlWireReader } from "./mysqlReader.cp";

/// MySQL `COM_STMT_EXECUTE` 命令字节 (`0x17`)
pub const MYSQL_COM_STMT_EXECUTE_CMD: u8 = 0x17;

/// 零堆分配单条预处理执行帧支持的最大参数数量（对应 8 字节 NULL 位图）
pub const MYSQL_STMT_MAX_PARAMS: usize = 64;

/// 游标标志常量（MySQL Protocol::COM_STMT_EXECUTE）
pub const MYSQL_CURSOR_TYPE_NO_CURSOR: u8 = 0x00;
pub const MYSQL_CURSOR_TYPE_READ_ONLY: u8 = 0x01;
pub const MYSQL_CURSOR_TYPE_FOR_UPDATE: u8 = 0x02;
pub const MYSQL_CURSOR_TYPE_SCROLLABLE: u8 = 0x04;

/// 参数类型头部的无符号标志位（第二字节 `0x80`）
pub const MYSQL_PARAM_UNSIGNED_FLAG: u8 = 0x80;

/// MySQL `COM_STMT_EXECUTE` 编解码错误集
pub err MySqlStmtExecuteError {
    BufferTooSmall,
    TooManyParams,
    InvalidCommandByte,
    TruncatedPayload,
    InvalidBindFlag,
    UnsupportedFieldType,
    MissingTypeMetadata,
    TrailingBytes,
}

/// MySQL 二进制协议预处理参数字段类型（对齐 `MySQLTypes.FieldType`）
pub enum MySqlExecFieldType: u8 {
    Tiny = 0x01,
    Short = 0x02,
    Long = 0x03,
    Float = 0x04,
    Double = 0x05,
    Null = 0x06,
    LongLong = 0x08,
    Json = 0xf5,
    NewDecimal = 0xf6,
    Blob = 0xfc,
    VarString = 0xfd,
    String = 0xfe,

    static fn fromByte(b: u8): MySqlExecFieldType! {
        return switch (b) {
            0x01 => MySqlExecFieldType.Tiny,
            0x02 => MySqlExecFieldType.Short,
            0x03 => MySqlExecFieldType.Long,
            0x04 => MySqlExecFieldType.Float,
            0x05 => MySqlExecFieldType.Double,
            0x06 => MySqlExecFieldType.Null,
            0x08 => MySqlExecFieldType.LongLong,
            0xf5 => MySqlExecFieldType.Json,
            0xf6 => MySqlExecFieldType.NewDecimal,
            0xfc => MySqlExecFieldType.Blob,
            0xfd => MySqlExecFieldType.VarString,
            0xfe => MySqlExecFieldType.String,
            else => MySqlStmtExecuteError.UnsupportedFieldType,
        };
    }

    /// 是否采用固定字节小端二进制编码（非 length-encoded string）
    fn isBinaryFixedWidth(): bool {
        return switch (_) {
            MySqlExecFieldType.Tiny => true,
            MySqlExecFieldType.Short => true,
            MySqlExecFieldType.Long => true,
            MySqlExecFieldType.Float => true,
            MySqlExecFieldType.Double => true,
            MySqlExecFieldType.LongLong => true,
            MySqlExecFieldType.Null => false,
            MySqlExecFieldType.Json => false,
            MySqlExecFieldType.NewDecimal => false,
            MySqlExecFieldType.Blob => false,
            MySqlExecFieldType.VarString => false,
            MySqlExecFieldType.String => false,
        };
    }
}

/// 零堆分配 MySQL 预处理语句绑定参数
pub struct MySqlExecParam {
    field_type: MySqlExecFieldType = MySqlExecFieldType.Null,
    is_unsigned: bool = false,
    is_null: bool = false,
    int_val: i64 = 0,
    uint_val: u64 = 0,
    float_val: f32 = 0.0,
    double_val: f64 = 0.0,
    bytes_val: []const u8 = "",

    static fn nullParam(): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Null, is_null: true };
    }

    static fn tiny(v: i8): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Tiny, int_val: v as i64 };
    }

    static fn tinyUnsigned(v: u8): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Tiny, is_unsigned: true, uint_val: v as u64 };
    }

    static fn short(v: i16): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Short, int_val: v as i64 };
    }

    static fn shortUnsigned(v: u16): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Short, is_unsigned: true, uint_val: v as u64 };
    }

    static fn long(v: i32): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Long, int_val: v as i64 };
    }

    static fn longUnsigned(v: u32): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Long, is_unsigned: true, uint_val: v as u64 };
    }

    static fn longLong(v: i64): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.LongLong, int_val: v };
    }

    static fn longLongUnsigned(v: u64): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.LongLong, is_unsigned: true, uint_val: v };
    }

    static fn float32(v: f32): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Float, float_val: v };
    }

    static fn float64(v: f64): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Double, double_val: v };
    }

    static fn text(s: []const u8): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.VarString, bytes_val: s };
    }

    static fn blob(b: []const u8): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Blob, bytes_val: b };
    }

    static fn json(s: []const u8): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.Json, bytes_val: s };
    }

    static fn decimal(s: []const u8): MySqlExecParam {
        return MySqlExecParam{ field_type: MySqlExecFieldType.NewDecimal, bytes_val: s };
    }
}

/// `COM_STMT_EXECUTE` 请求元数据
pub struct MySqlStmtExecuteRequest {
    statement_id: u32 = 0,
    flags: u8 = MYSQL_CURSOR_TYPE_NO_CURSOR,
    iteration_count: u32 = 1,
    new_params_bind_flag: bool = true,
}

/// 解码后的 `COM_STMT_EXECUTE` 完整视图（零拷贝借用原始输入缓冲区中的字符串/BLOB 切片）
pub struct MySqlStmtExecuteDecoded {
    statement_id: u32 = 0,
    flags: u8 = 0,
    iteration_count: u32 = 0,
    param_count: usize = 0,
    new_params_bind_flag: bool = false,
    null_bitmap: [8]u8 = [0; 8],
    params: [MYSQL_STMT_MAX_PARAMS]MySqlExecParam = [MySqlExecParam{}; MYSQL_STMT_MAX_PARAMS],

    fn isParamNull(idx: usize): bool {
        if (idx >= _.param_count) {
            return false;
        }
        return mysqlIsNullBitmapSet(_.null_bitmap[0..], idx);
    }
}

/// 计算 `COM_STMT_EXECUTE` NULL 位图字节长度（`(param_count + 7) / 8`，无偏移）
pub fn mysqlNullBitmapLen(param_count: usize): usize {
    return (param_count + 7) / 8;
}

/// 查询 NULL 位图中第 `idx` 个参数是否为 NULL
pub fn mysqlIsNullBitmapSet(bitmap: []const u8, idx: usize): bool {
    const byte_idx = idx >> 3;
    if (byte_idx >= bitmap.len) {
        return false;
    }
    const bit_mask: u8 = (1 as u8) << ((idx & 7) as u3);
    return (bitmap[byte_idx] & bit_mask) != 0;
}

fn writeStmtExecutePayloadInto(
    w: *MySqlWireWriter,
    req: MySqlStmtExecuteRequest,
    params: []const MySqlExecParam,
): void! {
    if (params.len > MYSQL_STMT_MAX_PARAMS) {
        return MySqlStmtExecuteError.TooManyParams;
    }

    w.writeU8(MYSQL_COM_STMT_EXECUTE_CMD) catch return MySqlStmtExecuteError.BufferTooSmall;
    w.writeU32(req.statement_id) catch return MySqlStmtExecuteError.BufferTooSmall;
    w.writeU8(req.flags) catch return MySqlStmtExecuteError.BufferTooSmall;
    w.writeU32(req.iteration_count) catch return MySqlStmtExecuteError.BufferTooSmall;

    if (params.len == 0) {
        return;
    }

    // 1. 构造并写入 NULL 位图（offset = 0）
    let bitmap: [8]u8 = [0; 8];
    const bitmap_len = mysqlNullBitmapLen(params.len);
    let i: usize = 0;
    while (i < params.len) {
        if (params[i].is_null || params[i].field_type == MySqlExecFieldType.Null) {
            const byte_idx = i >> 3;
            const bit_mask: u8 = (1 as u8) << ((i & 7) as u3);
            bitmap[byte_idx] |= bit_mask;
        }
        i += 1;
    }
    w.writeBytes(bitmap[0..bitmap_len]) catch return MySqlStmtExecuteError.BufferTooSmall;

    // 2. new_params_bind_flag (1B) + 可选参数类型列表 (2B * N)
    const bind_byte: u8 = if (req.new_params_bind_flag) 1 else 0;
    w.writeU8(bind_byte) catch return MySqlStmtExecuteError.BufferTooSmall;
    if (req.new_params_bind_flag) {
        let ti: usize = 0;
        while (ti < params.len) {
            const p = params[ti];
            const type_byte: u8 = if (p.is_null) (MySqlExecFieldType.Null as u8) else (p.field_type as u8);
            const flag_byte: u8 = if (p.is_unsigned) MYSQL_PARAM_UNSIGNED_FLAG else 0;
            w.writeU8(type_byte) catch return MySqlStmtExecuteError.BufferTooSmall;
            w.writeU8(flag_byte) catch return MySqlStmtExecuteError.BufferTooSmall;
            ti += 1;
        }
    }

    // 3. 写入所有非 NULL 参数值
    let vi: usize = 0;
    while (vi < params.len) {
        const p = params[vi];
        vi += 1;
        if (p.is_null || p.field_type == MySqlExecFieldType.Null) {
            continue;
        }
        switch (p.field_type) {
            MySqlExecFieldType.Tiny => {
                const b: u8 = if (p.is_unsigned) ((p.uint_val & 0xff) as u8) else ((p.int_val & 0xff) as u8);
                w.writeU8(b) catch return MySqlStmtExecuteError.BufferTooSmall;
            },
            MySqlExecFieldType.Short => {
                const v: u16 = if (p.is_unsigned) ((p.uint_val & 0xffff) as u16) else ((p.int_val & 0xffff) as u16);
                w.writeU16(v) catch return MySqlStmtExecuteError.BufferTooSmall;
            },
            MySqlExecFieldType.Long => {
                const v: u32 = if (p.is_unsigned) ((p.uint_val & 0xffff_ffff) as u32) else ((p.int_val & 0xffff_ffff) as u32);
                w.writeU32(v) catch return MySqlStmtExecuteError.BufferTooSmall;
            },
            MySqlExecFieldType.LongLong => {
                if (p.is_unsigned) {
                    w.writeU64(p.uint_val) catch return MySqlStmtExecuteError.BufferTooSmall;
                } else {
                    w.writeI64(p.int_val) catch return MySqlStmtExecuteError.BufferTooSmall;
                }
            },
            MySqlExecFieldType.Float => {
                w.writeF32(p.float_val) catch return MySqlStmtExecuteError.BufferTooSmall;
            },
            MySqlExecFieldType.Double => {
                w.writeF64(p.double_val) catch return MySqlStmtExecuteError.BufferTooSmall;
            },
            MySqlExecFieldType.VarString,
            MySqlExecFieldType.String,
            MySqlExecFieldType.Blob,
            MySqlExecFieldType.Json,
            MySqlExecFieldType.NewDecimal => {
                w.writeLenEncString(p.bytes_val) catch return MySqlStmtExecuteError.BufferTooSmall;
            },
            MySqlExecFieldType.Null => {},
        }
    }
}

/// 编码不含 4 字节包头的 `COM_STMT_EXECUTE` 原始载荷
pub fn encodeMySqlStmtExecutePayload(
    req: MySqlStmtExecuteRequest,
    params: []const MySqlExecParam,
    out: []u8,
): usize! {
    let w = MySqlWireWriter.init(out);
    writeStmtExecutePayloadInto(&w, req, params)?;
    return w.offset();
}

/// 编码完整 MySQL 线协议帧（4 字节包头 + `COM_STMT_EXECUTE` 载荷）
pub fn encodeMySqlStmtExecute(
    req: MySqlStmtExecuteRequest,
    params: []const MySqlExecParam,
    sequence_id: u8,
    out: []u8,
): usize! {
    let w = MySqlWireWriter.init(out);
    const patch = w.beginPacket(sequence_id) catch return MySqlStmtExecuteError.BufferTooSmall;
    writeStmtExecutePayloadInto(&w, req, params)?;
    * = w.finishPacket(patch) catch return MySqlStmtExecuteError.BufferTooSmall;
    return w.offset();
}

/// 解码 `COM_STMT_EXECUTE` 原始载荷（要求 `param_count == 0` 或 `new_params_bind_flag == 1`）
pub fn decodeMySqlStmtExecutePayload(payload: []const u8, param_count: usize): MySqlStmtExecuteDecoded! {
    if (param_count > MYSQL_STMT_MAX_PARAMS) {
        return MySqlStmtExecuteError.TooManyParams;
    }
    let r = MySqlWireReader.init(payload);
    const cmd = r.readU8() catch return MySqlStmtExecuteError.TruncatedPayload;
    if (cmd != MYSQL_COM_STMT_EXECUTE_CMD) {
        return MySqlStmtExecuteError.InvalidCommandByte;
    }

    let out = MySqlStmtExecuteDecoded{
        statement_id: r.readU32() catch return MySqlStmtExecuteError.TruncatedPayload,
        flags: r.readU8() catch return MySqlStmtExecuteError.TruncatedPayload,
        iteration_count: r.readU32() catch return MySqlStmtExecuteError.TruncatedPayload,
        param_count: param_count,
    };

    if (param_count == 0) {
        if (r.remaining() != 0) {
            return MySqlStmtExecuteError.TrailingBytes;
        }
        return out;
    }

    const bitmap_len = mysqlNullBitmapLen(param_count);
    const bm_slice = r.readBytes(bitmap_len) catch return MySqlStmtExecuteError.TruncatedPayload;
    let bi: usize = 0;
    while (bi < bitmap_len) {
        out.null_bitmap[bi] = bm_slice[bi];
        bi += 1;
    }

    const bind_flag = r.readU8() catch return MySqlStmtExecuteError.TruncatedPayload;
    if (bind_flag != 0 && bind_flag != 1) {
        return MySqlStmtExecuteError.InvalidBindFlag;
    }
    out.new_params_bind_flag = bind_flag == 1;
    if (!out.new_params_bind_flag) {
        return MySqlStmtExecuteError.MissingTypeMetadata;
    }

    // 读取参数类型与无符号标志
    let ti: usize = 0;
    while (ti < param_count) {
        const type_b = r.readU8() catch return MySqlStmtExecuteError.TruncatedPayload;
        const flag_b = r.readU8() catch return MySqlStmtExecuteError.TruncatedPayload;
        const ft = MySqlExecFieldType.fromByte(type_b)?;
        const is_null = mysqlIsNullBitmapSet(out.null_bitmap[0..bitmap_len], ti) || ft == MySqlExecFieldType.Null;
        out.params[ti] = MySqlExecParam{
            field_type: ft,
            is_unsigned: (flag_b & MYSQL_PARAM_UNSIGNED_FLAG) != 0,
            is_null: is_null,
        };
        ti += 1;
    }

    // 读取所有非 NULL 参数值
    let vi: usize = 0;
    while (vi < param_count) {
        if (out.params[vi].is_null) {
            vi += 1;
            continue;
        }
        const ft = out.params[vi].field_type;
        const uns = out.params[vi].is_unsigned;
        switch (ft) {
            MySqlExecFieldType.Tiny => {
                const raw = r.readU8() catch return MySqlStmtExecuteError.TruncatedPayload;
                if (uns) {
                    out.params[vi].uint_val = raw as u64;
                } else {
                    const signed_v: i8 = @bitCast(raw);
                    out.params[vi].int_val = signed_v as i64;
                }
            },
            MySqlExecFieldType.Short => {
                const raw = r.readU16() catch return MySqlStmtExecuteError.TruncatedPayload;
                if (uns) {
                    out.params[vi].uint_val = raw as u64;
                } else {
                    const signed_v: i16 = @bitCast(raw);
                    out.params[vi].int_val = signed_v as i64;
                }
            },
            MySqlExecFieldType.Long => {
                const raw = r.readU32() catch return MySqlStmtExecuteError.TruncatedPayload;
                if (uns) {
                    out.params[vi].uint_val = raw as u64;
                } else {
                    const signed_v: i32 = @bitCast(raw);
                    out.params[vi].int_val = signed_v as i64;
                }
            },
            MySqlExecFieldType.LongLong => {
                if (uns) {
                    out.params[vi].uint_val = r.readU64() catch return MySqlStmtExecuteError.TruncatedPayload;
                } else {
                    out.params[vi].int_val = r.readI64() catch return MySqlStmtExecuteError.TruncatedPayload;
                }
            },
            MySqlExecFieldType.Float => {
                out.params[vi].float_val = r.readF32() catch return MySqlStmtExecuteError.TruncatedPayload;
            },
            MySqlExecFieldType.Double => {
                out.params[vi].double_val = r.readF64() catch return MySqlStmtExecuteError.TruncatedPayload;
            },
            MySqlExecFieldType.VarString,
            MySqlExecFieldType.String,
            MySqlExecFieldType.Blob,
            MySqlExecFieldType.Json,
            MySqlExecFieldType.NewDecimal => {
                out.params[vi].bytes_val = r.readLenEncString() catch return MySqlStmtExecuteError.TruncatedPayload;
            },
            MySqlExecFieldType.Null => {},
        }
        vi += 1;
    }

    if (r.remaining() != 0) {
        return MySqlStmtExecuteError.TrailingBytes;
    }
    return out;
}

/// 解码完整 MySQL 线协议帧（校验 4 字节小端包头长度并解码内部 `COM_STMT_EXECUTE` 载荷）
pub fn decodeMySqlStmtExecuteFrame(frame: []const u8, param_count: usize): MySqlStmtExecuteDecoded! {
    if (frame.len < MYSQL_PACKET_HEADER_SIZE) {
        return MySqlStmtExecuteError.TruncatedPayload;
    }
    const payload_len: usize = (frame[0] as usize) | ((frame[1] as usize) << 8) | ((frame[2] as usize) << 16);
    if (MYSQL_PACKET_HEADER_SIZE + payload_len > frame.len) {
        return MySqlStmtExecuteError.TruncatedPayload;
    }
    if (MYSQL_PACKET_HEADER_SIZE + payload_len < frame.len) {
        return MySqlStmtExecuteError.TrailingBytes;
    }
    return decodeMySqlStmtExecutePayload(frame[MYSQL_PACKET_HEADER_SIZE .. MYSQL_PACKET_HEADER_SIZE + payload_len], param_count);
}
