// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/CommandComplete.cp | Synced: 2026-11-22T21:12:18Z
// Pure-CP zero-allocation PostgreSQL v3.0 Backend `CommandComplete` ('C') wire frame codec.
// Integrates with `PgCommandTag` for zero-alloc command classification, row-count extraction,
// transaction-control detection, and direct tag-to-frame serialization.

import {
    PgCommandKind,
    PgCommandTag,
} from "./commandTag.cp";

/// PostgreSQL Backend `CommandComplete` 报文类型字节（`'C' = 0x43`）
pub const PG_TAG_COMMAND_COMPLETE: u8 = 'C';

/// `CommandComplete` 最小完整帧长度（1B `'C'` + 4B length + 1B `'\0'` = 6）
pub const PG_COMMAND_COMPLETE_MIN_FRAME_LEN: usize = 6;

/// `CommandComplete` 编解码与边界校验错误集
pub err PgCommandCompleteError {
    InvalidTag,
    InvalidLength,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    EmbeddedNul,
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

fn hasEmbeddedNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn slicesEqual(a: []const u8, b: []const u8): bool {
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

/// 零堆分配 PostgreSQL v3.0 Backend `CommandComplete` (`'C'`) 线协议报文
///
/// 线格式：`['C'][len: i32 BE][command_tag_utf8][0x00]`
pub struct PgCommandCompleteFrame {
    raw_tag: []const u8 = "",

    /// 从原始命令标签文本构造 `CommandComplete` 帧
    static fn init(raw_tag: []const u8): PgCommandCompleteFrame {
        return PgCommandCompleteFrame{ raw_tag: raw_tag };
    }

    /// 校验标签中不含内嵌 `0x00` 字节
    fn validate(): PgCommandCompleteError!bool {
        if (hasEmbeddedNul(_.raw_tag)) {
            return PgCommandCompleteError.EmbeddedNul;
        }
        return true;
    }

    /// 不含 5 字节帧头的载荷长度（`raw_tag.len + 1`）
    fn payloadLen(): usize {
        return _.raw_tag.len + 1;
    }

    /// 含 5 字节帧头的完整帧长度（`5 + raw_tag.len + 1`）
    fn frameLen(): usize {
        return 5 + _.payloadLen();
    }

    /// 编码不含 5 字节帧头的 NUL 终结载荷到 `out`
    fn encodePayload(out: []u8): PgCommandCompleteError!usize {
        * = _.validate()?;
        const plen = _.payloadLen();
        if (out.len < plen) {
            return PgCommandCompleteError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < _.raw_tag.len) {
            out[i] = _.raw_tag[i];
            i += 1;
        }
        out[_.raw_tag.len] = 0;
        return plen;
    }

    /// 编码完整 `'C'` 帧到 `out`，返回写入字节数
    fn encodeFrame(out: []u8): PgCommandCompleteError!usize {
        * = _.validate()?;
        const flen = _.frameLen();
        if (out.len < flen) {
            return PgCommandCompleteError.BufferTooSmall;
        }
        const msg_len: u32 = @as(u32, @intCast(4 + _.payloadLen()));
        out[0] = PG_TAG_COMMAND_COMPLETE;
        writeBeU32(out, 1, msg_len);
        const written = _.encodePayload(out[5..])?;
        return 5 + written;
    }

    /// 直接将结构化 `PgCommandTag` 格式化并封帧到 `out`（零堆分配）
    static fn encodeFromTag(tag: PgCommandTag, out: []u8): PgCommandCompleteError!usize {
        if (out.len < PG_COMMAND_COMPLETE_MIN_FRAME_LEN) {
            return PgCommandCompleteError.BufferTooSmall;
        }
        const tag_len = tag.format(out[5..(out.len - 1)]) catch {
            return PgCommandCompleteError.BufferTooSmall;
        };
        if (hasEmbeddedNul(out[5..(5 + tag_len)])) {
            return PgCommandCompleteError.EmbeddedNul;
        }
        out[5 + tag_len] = 0;
        const msg_len: u32 = @as(u32, @intCast(4 + tag_len + 1));
        out[0] = PG_TAG_COMMAND_COMPLETE;
        writeBeU32(out, 1, msg_len);
        return 5 + tag_len + 1;
    }

    /// 解码不含 5 字节帧头的载荷（严格校验唯一 NUL 终结符与零尾随字节）
    static fn decodePayload(payload: []const u8): PgCommandCompleteError!PgCommandCompleteFrame {
        if (payload.len < 1) {
            return PgCommandCompleteError.Truncated;
        }
        let nul_pos: usize = 0;
        let found = false;
        while (nul_pos < payload.len) {
            if (payload[nul_pos] == 0) {
                found = true;
                break;
            }
            nul_pos += 1;
        }
        if (!found) {
            return PgCommandCompleteError.MissingNulTerminator;
        }
        if (nul_pos + 1 < payload.len) {
            return PgCommandCompleteError.TrailingBytes;
        }
        return PgCommandCompleteFrame{
            raw_tag: payload[0..nul_pos],
        };
    }

    /// 解码完整 `'C'` 帧
    static fn decodeFrame(frame: []const u8): PgCommandCompleteError!PgCommandCompleteFrame {
        if (frame.len < PG_COMMAND_COMPLETE_MIN_FRAME_LEN) {
            return PgCommandCompleteError.Truncated;
        }
        if (frame[0] != PG_TAG_COMMAND_COMPLETE) {
            return PgCommandCompleteError.InvalidTag;
        }
        const msg_len_u32 = readBeU32(frame, 1);
        if (msg_len_u32 < 5) {
            return PgCommandCompleteError.InvalidLength;
        }
        const total_len: usize = @as(usize, msg_len_u32) + 1;
        if (frame.len < total_len) {
            return PgCommandCompleteError.Truncated;
        }
        if (frame.len > total_len) {
            return PgCommandCompleteError.TrailingBytes;
        }
        return PgCommandCompleteFrame.decodePayload(frame[5..total_len]);
    }

    /// 解析为结构化 `PgCommandTag`
    fn parseTag(): PgCommandTag {
        return PgCommandTag.parse(_.raw_tag);
    }

    /// 返回命令分类枚举
    fn kind(): PgCommandKind {
        return _.parseTag().kind;
    }

    /// 返回影响/返回行数（无行数的命令返回 `null`）
    fn rowCount(): u64? {
        return _.parseTag().rowCount();
    }

    /// 判断是否为 DML 写入命令（`INSERT`/`DELETE`/`UPDATE`/`MERGE`）
    fn isDml(): bool {
        return _.parseTag().isDml();
    }

    /// 判断是否为查询/游标命令（`SELECT`/`MOVE`/`FETCH`）
    fn isQueryOrCursor(): bool {
        return _.parseTag().isQueryOrCursor();
    }

    /// 判断是否为事务控制命令（`BEGIN`/`COMMIT`/`ROLLBACK`/`SAVEPOINT`/`RELEASE`/`PREPARE TRANSACTION`）
    fn isTransactionControl(): bool {
        return slicesEqual(_.raw_tag, "BEGIN") ||
            slicesEqual(_.raw_tag, "COMMIT") ||
            slicesEqual(_.raw_tag, "ROLLBACK") ||
            slicesEqual(_.raw_tag, "SAVEPOINT") ||
            slicesEqual(_.raw_tag, "RELEASE") ||
            slicesEqual(_.raw_tag, "PREPARE TRANSACTION");
    }

    /// 值语义相等比较
    fn eql(other: PgCommandCompleteFrame): bool {
        return slicesEqual(_.raw_tag, other.raw_tag);
    }
}
