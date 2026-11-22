// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/NegotiateProtocolVersion.cp | Synced: 2026-11-22T04:14:47Z
// Pure-CP zero-allocation PostgreSQL v3.0 `NegotiateProtocolVersion` ('v') message codec.

/// PostgreSQL Backend `NegotiateProtocolVersion` 报文类型字节（`'v' = 0x76`）
pub const PG_TAG_NEGOTIATE_PROTOCOL_VERSION: u8 = 'v';

/// 单条 `NegotiateProtocolVersion` 报文允许的最大未识别选项数量（防资源耗尽上限）
pub const PG_NEGOTIATE_MAX_OPTIONS: usize = 32;

/// `NegotiateProtocolVersion` 编解码与边界校验错误集
pub err PgNegotiateError {
    InvalidTag,
    InvalidLength,
    Truncated,
    TooManyOptions,
    TrailingBytes,
    BufferTooSmall,
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// PostgreSQL v3.0 `NegotiateProtocolVersion` 服务端响应报文（Tag `'v'`）
///
/// 线协议格式（大端序）：
/// - `Byte1('v')`：报文类型标识
/// - `Int32`：包含自身在内的报文长度（`= 4 + payload.len`）
/// - `Int32`：服务端支持的最新次版本号（`newest_minor_version`）
/// - `Int32`：未识别的启动选项数量（`N`）
/// - `String[N]`：`N` 个以 `0x00` 结尾的未识别协议选项名
pub struct PgNegotiateProtocolVersion {
    newest_minor_version: u32 = 0,
    options: [PG_NEGOTIATE_MAX_OPTIONS][]const u8 = [""; PG_NEGOTIATE_MAX_OPTIONS],
    option_count: usize = 0,

    static fn init(newest_minor_version: u32): PgNegotiateProtocolVersion {
        return PgNegotiateProtocolVersion{
            newest_minor_version: newest_minor_version,
        };
    }

    /// 追加一个未识别协议选项名（超过 `PG_NEGOTIATE_MAX_OPTIONS` 时返回 `TooManyOptions`）
    fn addOption(*_, opt: []const u8): usize! {
        if (_.option_count >= PG_NEGOTIATE_MAX_OPTIONS) {
            return err.TooManyOptions;
        }
        const idx = _.option_count;
        _.options[idx] = opt;
        _.option_count += 1;
        return idx;
    }

    fn count(): usize {
        return _.option_count;
    }

    fn optionAt(idx: usize): ([]const u8)? {
        if (idx >= _.option_count) {
            return null;
        }
        return _.options[idx];
    }

    fn containsOption(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.option_count) {
            if (bytesEql(_.options[i], name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 若 `newest_minor_version` 高 16 位携带主版本则返回高 16 位，否则默认 PostgreSQL 主版本 3
    fn majorVersion(): u16 {
        const hi = @as(u16, @intCast((_.newest_minor_version >> 16) & 0xFFFF));
        if (hi == 0) {
            return 3;
        }
        return hi;
    }

    /// 返回低 16 位次版本号
    fn minorVersion(): u16 {
        return @as(u16, @intCast(_.newest_minor_version & 0xFFFF));
    }

    /// 计算不含 `'v'` 与 4 字节长度头的载荷字节数
    fn encodedPayloadLen(): usize {
        let total: usize = 8;
        let i: usize = 0;
        while (i < _.option_count) {
            total += _.options[i].len + 1;
            i += 1;
        }
        return total;
    }

    /// 计算完整线协议帧字节数（含 `'v'` 与 4 字节大端序长度头）
    fn encodedFrameLen(): usize {
        return 5 + _.encodedPayloadLen();
    }

    /// 将载荷编码为大端序二进制字节流
    fn encodePayload(out: []u8): usize! {
        const need = _.encodedPayloadLen();
        if (out.len < need) {
            return err.BufferTooSmall;
        }
        writeBeU32(out, 0, _.newest_minor_version);
        writeBeU32(out, 4, @as(u32, @intCast(_.option_count)));
        let pos: usize = 8;
        let i: usize = 0;
        while (i < _.option_count) {
            const opt = _.options[i];
            let j: usize = 0;
            while (j < opt.len) {
                out[pos + j] = opt[j];
                j += 1;
            }
            pos += opt.len;
            out[pos] = 0;
            pos += 1;
            i += 1;
        }
        return pos;
    }

    /// 编码完整 PostgreSQL Backend `'v'` 帧（含 1 字节 Tag 与 4 字节长度头）
    fn encodeFrame(out: []u8): usize! {
        const need = _.encodedFrameLen();
        if (out.len < need) {
            return err.BufferTooSmall;
        }
        const payload_len = need - 5;
        out[0] = PG_TAG_NEGOTIATE_PROTOCOL_VERSION;
        writeBeU32(out, 1, @as(u32, @intCast(4 + payload_len)));
        const written = _.encodePayload(out[5..need])?;
        return 5 + written;
    }

    /// 零拷贝解码 `NegotiateProtocolVersion` 载荷（不含 `'v'` 与 4 字节长度头）
    static fn decodePayload(payload: []const u8): PgNegotiateProtocolVersion! {
        if (payload.len < 8) {
            return err.Truncated;
        }
        const version = readBeU32(payload, 0);
        const raw_count = readBeU32(payload, 4);
        // 防护负数（作为有符号 Int32 最高位为 1）或超过固定上限的异常计数
        if ((raw_count & 0x80000000) != 0 || @as(usize, raw_count) > PG_NEGOTIATE_MAX_OPTIONS) {
            return err.TooManyOptions;
        }
        const opt_count: usize = @as(usize, raw_count);
        let msg = PgNegotiateProtocolVersion.init(version);
        let pos: usize = 8;
        let idx: usize = 0;
        while (idx < opt_count) {
            let nul_pos = pos;
            while (nul_pos < payload.len && payload[nul_pos] != 0) {
                nul_pos += 1;
            }
            if (nul_pos >= payload.len) {
                return err.Truncated;
            }
            msg.options[idx] = payload[pos..nul_pos];
            msg.option_count += 1;
            pos = nul_pos + 1;
            idx += 1;
        }
        if (pos != payload.len) {
            return err.TrailingBytes;
        }
        return msg;
    }

    /// 零拷贝解码完整 PostgreSQL `'v'` 帧（严格校验 Tag 与大端序长度字段）
    static fn decodeFrame(frame: []const u8): PgNegotiateProtocolVersion! {
        if (frame.len < 5) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_NEGOTIATE_PROTOCOL_VERSION) {
            return err.InvalidTag;
        }
        const body_len = readBeU32(frame, 1);
        if (body_len < 12) {
            return err.InvalidLength;
        }
        const expected_total: usize = @as(usize, body_len) + 1;
        if (expected_total != frame.len) {
            return err.InvalidLength;
        }
        return PgNegotiateProtocolVersion.decodePayload(frame[5..frame.len]);
    }

    fn eql(other: PgNegotiateProtocolVersion): bool {
        if (_.newest_minor_version != other.newest_minor_version || _.option_count != other.option_count) {
            return false;
        }
        let i: usize = 0;
        while (i < _.option_count) {
            if (!bytesEql(_.options[i], other.options[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}
