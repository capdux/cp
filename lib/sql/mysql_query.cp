// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/Query.cp | Synced: 2026-11-22T05:25:00Z
// ============================================================================
// Pure-CP Zero-Allocation MySQL COM_QUERY & 8.0.23+ Query Attributes Codec
// Absorbed from Bun: src/sql/mysql/protocol/Query.cp
// ============================================================================

import {
    MYSQL_PACKET_HEADER_SIZE,
    MySqlWireWriter,
} from "./mysql_wire_writer.cp";
import { MySqlWireReader } from "./mysql_wire_reader.cp";

/// MySQL `COM_QUERY` 命令码 (`0x03`)
pub const MYSQL_COM_QUERY_CMD: u8 = 0x03;

/// 单次查询属性最大参数数量上限（防异常大包资源耗尽）
pub const MYSQL_MAX_QUERY_PARAMS: usize = 64;

/// MySQL `COM_QUERY` 编解码错误集
pub err MySqlQueryCodecError {
    BufferTooSmall,
    EmptyQuery,
    TooManyParameters,
    InvalidPacket,
    InvalidCommand,
    TruncatedPayload,
    ParameterMismatch,
}

/// MySQL 查询参数线协议类型码
pub enum MySqlQueryParamKind: u8 {
    int32 = 0x03,
    float64 = 0x05,
    null_val = 0x06,
    int64 = 0x08,
    blob = 0xfc,
    string = 0xfe,

    /// 是否采用定长小端二进制编码（否则按 Length-Encoded String 编码）
    fn isBinaryFixed(): bool {
        return _ == MySqlQueryParamKind.int32 ||
            _ == MySqlQueryParamKind.int64 ||
            _ == MySqlQueryParamKind.float64;
    }
}

/// 单条 MySQL 查询属性参数（支持命名与自动 `:pN` 占位名）
pub struct MySqlQueryParam {
    kind: MySqlQueryParamKind = MySqlQueryParamKind.null_val,
    is_unsigned: bool = false,
    name: []const u8 = "",
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    bytes_val: []const u8 = "",

    static fn nullParam(name: []const u8): MySqlQueryParam {
        return MySqlQueryParam{
            kind: MySqlQueryParamKind.null_val,
            name: name,
        };
    }

    static fn int32Param(name: []const u8, v: i32): MySqlQueryParam {
        return MySqlQueryParam{
            kind: MySqlQueryParamKind.int32,
            name: name,
            int_val: @intCast(v),
        };
    }

    static fn int64Param(name: []const u8, v: i64): MySqlQueryParam {
        return MySqlQueryParam{
            kind: MySqlQueryParamKind.int64,
            name: name,
            int_val: v,
        };
    }

    static fn uint64Param(name: []const u8, v: u64): MySqlQueryParam {
        return MySqlQueryParam{
            kind: MySqlQueryParamKind.int64,
            is_unsigned: true,
            name: name,
            int_val: @bitCast(v),
        };
    }

    static fn float64Param(name: []const u8, v: f64): MySqlQueryParam {
        return MySqlQueryParam{
            kind: MySqlQueryParamKind.float64,
            name: name,
            float_val: v,
        };
    }

    static fn stringParam(name: []const u8, s: []const u8): MySqlQueryParam {
        return MySqlQueryParam{
            kind: MySqlQueryParamKind.string,
            name: name,
            bytes_val: s,
        };
    }

    static fn blobParam(name: []const u8, b: []const u8): MySqlQueryParam {
        return MySqlQueryParam{
            kind: MySqlQueryParamKind.blob,
            name: name,
            bytes_val: b,
        };
    }
}

/// 零拷贝解码后的 `COM_QUERY` 报文视图
pub struct MySqlDecodedQuery {
    sequence_id: u8,
    payload_len: usize,
    query: []const u8,
    param_count: usize,
}

/// 编码标准 MySQL `COM_QUERY` 线协议报文（4B 包头 + `0x03` + SQL 文本），零堆分配
pub fn encodeMySqlSimpleQuery(
    buf: []u8,
    seq_id: u8,
    query: []const u8,
): []const u8! {
    if (query.len == 0) {
        return err.EmptyQuery;
    }
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(seq_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(MYSQL_COM_QUERY_CMD) catch {
        return err.BufferTooSmall;
    };
    * = w.writeBytes(query) catch {
        return err.BufferTooSmall;
    };
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 编码 MySQL 8.0.23+ `CLIENT_QUERY_ATTRIBUTES` 扩展 `COM_QUERY` 报文（零堆分配）
///
/// 报文布局：`[4B header][0x03][lenenc param_count][lenenc param_set_count=1]`
/// +（若 `params.len > 0`：`[null_bitmap][new_params_bind_flag=1][type+flag+lenenc_name...][values...]`）
/// + `[query bytes]`。
/// 当参数的 `name` 为空时，按 Bun 约定自动生成 `:p1`、`:p2`…
pub fn encodeMySqlQueryWithAttrs(
    buf: []u8,
    seq_id: u8,
    query: []const u8,
    params: []const MySqlQueryParam,
): []const u8! {
    if (query.len == 0) {
        return err.EmptyQuery;
    }
    if (params.len > MYSQL_MAX_QUERY_PARAMS) {
        return err.TooManyParameters;
    }

    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(seq_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(MYSQL_COM_QUERY_CMD) catch {
        return err.BufferTooSmall;
    };
    * = w.writeLenEncInt(@intCast(params.len)) catch {
        return err.BufferTooSmall;
    };
    // parameter_set_count 固定为 1（MySQL 8.0.23+ 协议规范）
    * = w.writeLenEncInt(1) catch {
        return err.BufferTooSmall;
    };

    if (params.len > 0) {
        // 1. NULL 位图
        let null_flags: [MYSQL_MAX_QUERY_PARAMS]bool = [false; MYSQL_MAX_QUERY_PARAMS];
        let i: usize = 0;
        while (i < params.len) {
            null_flags[i] = params[i].kind == MySqlQueryParamKind.null_val;
            i += 1;
        }
        * = w.writeNullBitmap(null_flags[0..params.len]) catch {
            return err.BufferTooSmall;
        };

        // 2. new_params_bind_flag = 1
        * = w.writeByte(1) catch {
            return err.BufferTooSmall;
        };

        // 3. 参数类型 + 无符号标志 + 长度编码参数名
        let name_buf: [24]u8 = [0; 24];
        let m: usize = 0;
        while (m < params.len) {
            const p = params[m];
            const param_idx = m + 1;
            m += 1;
            * = w.writeByte(@intFromEnum(p.kind)) catch {
                return err.BufferTooSmall;
            };
            const uflag: u8 = if (p.is_unsigned) 0x80 else 0x00;
            * = w.writeByte(uflag) catch {
                return err.BufferTooSmall;
            };
            const eff_name = if (p.name.len > 0)
                p.name
            else
                (std.fmt.bufPrint(name_buf[0..], ":p{d}", .{param_idx}) catch ":p");
            * = w.writeLenEncString(eff_name) catch {
                return err.BufferTooSmall;
            };
        }

        // 4. 非 NULL 参数值载荷
        let v: usize = 0;
        while (v < params.len) {
            const p = params[v];
            v += 1;
            if (p.kind == MySqlQueryParamKind.null_val) {
                continue;
            }
            switch (p.kind) {
                .int32 => {
                    * = w.writeI32(@intCast(p.int_val)) catch {
                        return err.BufferTooSmall;
                    };
                },
                .int64 => {
                    * = w.writeI64(p.int_val) catch {
                        return err.BufferTooSmall;
                    };
                },
                .float64 => {
                    * = w.writeF64(p.float_val) catch {
                        return err.BufferTooSmall;
                    };
                },
                .string, .blob => {
                    * = w.writeLenEncString(p.bytes_val) catch {
                        return err.BufferTooSmall;
                    };
                },
                .null_val => {},
            }
        }
    }

    * = w.writeBytes(query) catch {
        return err.BufferTooSmall;
    };
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 零拷贝解码标准 `COM_QUERY` 报文
pub fn decodeMySqlSimpleQuery(packet: []const u8): MySqlDecodedQuery! {
    if (packet.len < MYSQL_PACKET_HEADER_SIZE + 1) {
        return err.InvalidPacket;
    }
    let r = MySqlWireReader.init(packet);
    const payload_len: usize = @intCast(r.readU24() catch {
        return err.InvalidPacket;
    });
    const seq_id = r.readByte() catch {
        return err.InvalidPacket;
    };
    if (payload_len == 0 || r.remaining() < payload_len) {
        return err.TruncatedPayload;
    }
    const cmd = r.readByte() catch {
        return err.TruncatedPayload;
    };
    if (cmd != MYSQL_COM_QUERY_CMD) {
        return err.InvalidCommand;
    }
    const query_len = payload_len - 1;
    const q = r.readBytes(query_len) catch {
        return err.TruncatedPayload;
    };
    return MySqlDecodedQuery{
        sequence_id: seq_id,
        payload_len: payload_len,
        query: q,
        param_count: 0,
    };
}

/// 零拷贝解码 MySQL 8.0.23+ Query Attributes 扩展 `COM_QUERY` 报文
pub fn decodeMySqlQueryWithAttrs(packet: []const u8): MySqlDecodedQuery! {
    if (packet.len < MYSQL_PACKET_HEADER_SIZE + 3) {
        return err.InvalidPacket;
    }
    let r = MySqlWireReader.init(packet);
    const payload_len: usize = @intCast(r.readU24() catch {
        return err.InvalidPacket;
    });
    const seq_id = r.readByte() catch {
        return err.InvalidPacket;
    };
    if (payload_len < 3 || r.remaining() < payload_len) {
        return err.TruncatedPayload;
    }
    const payload_slice = r.readBytes(payload_len) catch {
        return err.TruncatedPayload;
    };

    let pr = MySqlWireReader.init(payload_slice);
    const cmd = pr.readByte() catch {
        return err.TruncatedPayload;
    };
    if (cmd != MYSQL_COM_QUERY_CMD) {
        return err.InvalidCommand;
    }

    const param_count_u64 = pr.readLenEncInt() catch {
        return err.TruncatedPayload;
    };
    const param_set_count = pr.readLenEncInt() catch {
        return err.TruncatedPayload;
    };
    if (param_set_count != 1) {
        return err.InvalidPacket;
    }
    if (param_count_u64 > MYSQL_MAX_QUERY_PARAMS) {
        return err.TooManyParameters;
    }
    const param_count: usize = @intCast(param_count_u64);

    if (param_count > 0) {
        const bitmap_len = (param_count + 7) / 8;
        const bitmap = pr.readBytes(bitmap_len) catch {
            return err.TruncatedPayload;
        };
        const bind_flag = pr.readByte() catch {
            return err.TruncatedPayload;
        };
        if (bind_flag != 1) {
            return err.ParameterMismatch;
        }

        let types: [MYSQL_MAX_QUERY_PARAMS]u8 = [0; MYSQL_MAX_QUERY_PARAMS];
        let i: usize = 0;
        while (i < param_count) {
            types[i] = pr.readByte() catch {
                return err.TruncatedPayload;
            };
            * = pr.readByte() catch {
                return err.TruncatedPayload;
            };
            * = pr.readLenEncString() catch {
                return err.TruncatedPayload;
            };
            i += 1;
        }

        let v: usize = 0;
        while (v < param_count) {
            const is_null = (bitmap[v >> 3] & (@as(u8, 1) << @as(u3, @truncate(v & 7)))) != 0;
            const t = types[v];
            v += 1;
            if (is_null || t == @intFromEnum(MySqlQueryParamKind.null_val)) {
                continue;
            }
            if (t == @intFromEnum(MySqlQueryParamKind.int32)) {
                * = pr.readBytes(4) catch {
                    return err.TruncatedPayload;
                };
            } else if (t == @intFromEnum(MySqlQueryParamKind.int64) ||
                t == @intFromEnum(MySqlQueryParamKind.float64))
            {
                * = pr.readBytes(8) catch {
                    return err.TruncatedPayload;
                };
            } else {
                * = pr.readLenEncString() catch {
                    return err.TruncatedPayload;
                };
            }
        }
    }

    const q = pr.peek();
    if (q.len == 0) {
        return err.EmptyQuery;
    }
    return MySqlDecodedQuery{
        sequence_id: seq_id,
        payload_len: payload_len,
        query: q,
        param_count: param_count,
    };
}
