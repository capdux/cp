// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/ParameterDescription.cp | Synced: 2026-11-22T21:01:07Z
// Pure-CP zero-allocation PostgreSQL v3.0 Backend `ParameterDescription` ('t') wire codec.
// Provides zero-copy OID slice view/iterator for arbitrary parameter counts, plus
// stack-backed encoder and slice-based encoder with strict framing & bounds checks.

/// PostgreSQL `ParameterDescription` 后端报文 Tag 字节 (`'t' = 0x74`)
pub const PG_TAG_PARAMETER_DESCRIPTION: u8 = 't';

/// `ParameterDescription` 最小完整帧长度（1 tag + 4 length + 2 count）
pub const PG_PARAM_DESC_HEADER_LEN: usize = 7;

/// `PgParamDescEncoder` 栈缓冲最大参数数量（`encodeFrameFromSlice` 无此上限）
pub const PG_PARAM_DESC_MAX_OIDS: usize = 64;

/// `ParameterDescription` 编解码与边界校验错误集
pub err PgParamDescError {
    InvalidTag,
    InvalidLength,
    NegativeCount,
    Truncated,
    TrailingBytes,
    BufferTooSmall,
    TooManyParams,
    IndexOutOfBounds,
}

fn readBeU16(buf: []const u8, off: usize): u16 {
    const b0: u16 = @as(u16, buf[off]);
    const b1: u16 = @as(u16, buf[off + 1]);
    return (b0 << 8) | b1;
}

fn readBeI16(buf: []const u8, off: usize): i16 {
    const u: u16 = readBeU16(buf, off);
    return @bitCast(u);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeBeU16(out: []u8, off: usize, v: u16) {
    out[off] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 1] = @as(u8, @intCast(v & 0xFF));
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
}

/// 零拷贝 `ParameterDescription` OID 迭代器
pub struct PgParamDescIter {
    oids_bytes: []const u8 = "",
    index: usize = 0,
    count: usize = 0,

    /// 读取下一个参数的 32 位 PostgreSQL Type OID，结束返回 `null`
    fn next(*_): u32? {
        if (_.index >= _.count) {
            return null;
        }
        const oid = readBeU32(_.oids_bytes, _.index * 4);
        _.index += 1;
        return oid;
    }

    /// 返回剩余未迭代的参数数量
    fn remaining(): usize {
        if (_.index >= _.count) {
            return 0;
        }
        return _.count - _.index;
    }

    /// 重置迭代器到首元素
    fn reset(*_) {
        _.index = 0;
    }
}

/// 零拷贝 PostgreSQL `ParameterDescription` (`'t'`) 解码视图
pub struct PgParamDescView {
    count: usize = 0,
    oids_bytes: []const u8 = "",

    /// 解码不含 5 字节帧头的载荷（`Int16 count` + `count * Int32 OID`）
    static fn decodePayload(payload: []const u8): PgParamDescError!PgParamDescView {
        if (payload.len < 2) {
            return PgParamDescError.Truncated;
        }
        const raw_count = readBeI16(payload, 0);
        if (raw_count < 0) {
            return PgParamDescError.NegativeCount;
        }
        const count: usize = @intCast(raw_count);
        const expected_len = 2 + count * 4;
        if (payload.len < expected_len) {
            return PgParamDescError.Truncated;
        }
        if (payload.len > expected_len) {
            return PgParamDescError.TrailingBytes;
        }
        return PgParamDescView{
            count: count,
            oids_bytes: payload[2..expected_len],
        };
    }

    /// 解码完整 `ParameterDescription` 帧（`'t'` + `Int32 length` + 载荷）
    static fn decodeFrame(frame: []const u8): PgParamDescError!PgParamDescView {
        if (frame.len < PG_PARAM_DESC_HEADER_LEN) {
            return PgParamDescError.Truncated;
        }
        if (frame[0] != PG_TAG_PARAMETER_DESCRIPTION) {
            return PgParamDescError.InvalidTag;
        }
        const msg_len_u32 = readBeU32(frame, 1);
        if (msg_len_u32 < 6) {
            return PgParamDescError.InvalidLength;
        }
        const total_len: usize = @as(usize, msg_len_u32) + 1;
        if (frame.len < total_len) {
            return PgParamDescError.Truncated;
        }
        if (frame.len > total_len) {
            return PgParamDescError.TrailingBytes;
        }
        return PgParamDescView.decodePayload(frame[5..total_len]);
    }

    /// 判断是否无参数
    fn isEmpty(): bool {
        return _.count == 0;
    }

    /// 读取第 `index`（0-based）个参数的 Type OID
    fn oidAt(index: usize): PgParamDescError!u32 {
        if (index >= _.count) {
            return PgParamDescError.IndexOutOfBounds;
        }
        return readBeU32(_.oids_bytes, index * 4);
    }

    /// 创建零拷贝 OID 迭代器
    fn iter(): PgParamDescIter {
        return PgParamDescIter{
            oids_bytes: _.oids_bytes,
            index: 0,
            count: _.count,
        };
    }

    /// 批量解出全部 OID 到调用方提供的 `dst` 切片，返回写入数量
    fn copyOids(dst: []u32): PgParamDescError!usize {
        if (dst.len < _.count) {
            return PgParamDescError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < _.count) {
            dst[i] = readBeU32(_.oids_bytes, i * 4);
            i += 1;
        }
        return _.count;
    }

    /// 检查是否包含指定 Type OID
    fn containsOid(target: u32): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (readBeU32(_.oids_bytes, i * 4) == target) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 检查是否存在未指定类型（`OID == 0`）的参数
    fn hasUnspecifiedOid(): bool {
        return _.containsOid(0);
    }
}

/// 零堆分配 PostgreSQL `ParameterDescription` (`'t'`) 编码器
pub struct PgParamDescEncoder {
    count: usize = 0,
    oids: [64]u32 = [0; 64],

    /// 构造空 `ParameterDescription` 编码器
    static fn init(): PgParamDescEncoder {
        return PgParamDescEncoder{};
    }

    /// 追加一个参数 Type OID，返回追加后的参数总数
    fn addOid(*_, oid: u32): PgParamDescError!usize {
        if (_.count >= PG_PARAM_DESC_MAX_OIDS) {
            return PgParamDescError.TooManyParams;
        }
        _.oids[_.count] = oid;
        _.count += 1;
        return _.count;
    }

    /// 不含 5 字节帧头的载荷长度（`2 + count * 4`）
    fn payloadLen(): usize {
        return 2 + _.count * 4;
    }

    /// 含 5 字节帧头的完整帧长度（`7 + count * 4`）
    fn frameLen(): usize {
        return PG_PARAM_DESC_HEADER_LEN + _.count * 4;
    }

    /// 编码载荷到 `out`
    fn encodePayload(out: []u8): PgParamDescError!usize {
        const needed = _.payloadLen();
        if (out.len < needed) {
            return PgParamDescError.BufferTooSmall;
        }
        writeBeU16(out, 0, @as(u16, @intCast(_.count)));
        let i: usize = 0;
        while (i < _.count) {
            writeBeU32(out, 2 + i * 4, _.oids[i]);
            i += 1;
        }
        return needed;
    }

    /// 编码完整 `'t'` 帧到 `out`
    fn encodeFrame(out: []u8): PgParamDescError!usize {
        return PgParamDescEncoder.encodeFrameFromSlice(_.oids[0.._.count], out);
    }

    /// 直接从任意长度 `oids` 切片零堆分配编码完整 `'t'` 帧（上限 32767 个参数，受协议 `Int16` 限制）
    static fn encodeFrameFromSlice(oids: []const u32, out: []u8): PgParamDescError!usize {
        if (oids.len > 32767) {
            return PgParamDescError.TooManyParams;
        }
        const total_len = PG_PARAM_DESC_HEADER_LEN + oids.len * 4;
        if (out.len < total_len) {
            return PgParamDescError.BufferTooSmall;
        }
        const msg_len: u32 = @as(u32, @intCast(4 + 2 + oids.len * 4));
        out[0] = PG_TAG_PARAMETER_DESCRIPTION;
        writeBeU32(out, 1, msg_len);
        writeBeU16(out, 5, @as(u16, @intCast(oids.len)));
        let i: usize = 0;
        while (i < oids.len) {
            writeBeU32(out, PG_PARAM_DESC_HEADER_LEN + i * 4, oids[i]);
            i += 1;
        }
        return total_len;
    }
}
