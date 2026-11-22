// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/CopyFail.cp | Synced: 2026-11-22T16:11:00Z
// ============================================================================
// lib/pgwire/copyFrames.cp — Zero-Alloc PostgreSQL v3.0 COPY Protocol Frames
// (`CopyData` 'd', `CopyDone` 'c', `CopyFail` 'f')
// ============================================================================

pub const PG_TAG_COPY_DATA: u8 = 'd';
pub const PG_TAG_COPY_DONE: u8 = 'c';
pub const PG_TAG_COPY_FAIL: u8 = 'f';
pub const PG_COPY_DONE_FRAME_LEN: usize = 5;

pub err PgCopyError {
    InvalidTag,
    InvalidLength,
    Truncated,
    MissingNulTerminator,
    EmbeddedNul,
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
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) return true;
        i += 1;
    }
    return false;
}

/// PostgreSQL v3.0 `CopyData` message (Tag `'d'`).
/// Carries arbitrary binary or text COPY stream chunk bytes (may include `0x00`).
pub struct PgCopyData {
    data: []const u8 = "",

    static fn init(data: []const u8): PgCopyData {
        return PgCopyData{ data: data };
    }

    fn isEmpty(): bool {
        return _.data.len == 0;
    }

    fn encodedFrameLen(): usize {
        return 5 + _.data.len;
    }

    fn encodeFrame(out: []u8): PgCopyError!usize {
        const total = _.encodedFrameLen();
        if (out.len < total) return PgCopyError.BufferTooSmall;
        const wire_len: u32 = @as(u32, @intCast(4 + _.data.len));
        out[0] = PG_TAG_COPY_DATA;
        writeBeU32(out, 1, wire_len);
        let i: usize = 0;
        while (i < _.data.len) {
            out[5 + i] = _.data[i];
            i += 1;
        }
        return total;
    }

    static fn decodeFrame(frame: []const u8): PgCopyError!PgCopyData {
        if (frame.len < 5) return PgCopyError.Truncated;
        if (frame[0] != PG_TAG_COPY_DATA) return PgCopyError.InvalidTag;
        const wire_len = readBeU32(frame, 1);
        if (wire_len < 4) return PgCopyError.InvalidLength;
        const total: usize = (@as(usize, wire_len)) + 1;
        if (frame.len < total) return PgCopyError.Truncated;
        if (frame.len > total) return PgCopyError.TrailingBytes;
        return PgCopyData{ data: frame[5..total] };
    }

    fn eql(other: PgCopyData): bool {
        return bytesEql(_.data, other.data);
    }
}

/// PostgreSQL v3.0 `CopyDone` message (Tag `'c'`, fixed 5-byte frame).
pub struct PgCopyDone {
    static fn encodeFrame(out: []u8): PgCopyError!usize {
        if (out.len < PG_COPY_DONE_FRAME_LEN) return PgCopyError.BufferTooSmall;
        out[0] = PG_TAG_COPY_DONE;
        writeBeU32(out, 1, 4);
        return PG_COPY_DONE_FRAME_LEN;
    }

    static fn decodeFrame(frame: []const u8): PgCopyError!bool {
        if (frame.len < PG_COPY_DONE_FRAME_LEN) return PgCopyError.Truncated;
        if (frame[0] != PG_TAG_COPY_DONE) return PgCopyError.InvalidTag;
        const wire_len = readBeU32(frame, 1);
        if (wire_len != 4) return PgCopyError.InvalidLength;
        if (frame.len > PG_COPY_DONE_FRAME_LEN) return PgCopyError.TrailingBytes;
        return true;
    }
}

/// PostgreSQL v3.0 `CopyFail` Frontend abort message (Tag `'f'`).
/// Wire format: `'f'` + Int32(`4 + message.len + 1`) + NUL-terminated error reason.
pub struct PgCopyFail {
    message: []const u8 = "",

    static fn init(message: []const u8): PgCopyFail {
        return PgCopyFail{ message: message };
    }

    fn validate(): PgCopyError!bool {
        if (containsNul(_.message)) return PgCopyError.EmbeddedNul;
        return true;
    }

    fn encodedPayloadLen(): usize {
        return _.message.len + 1;
    }

    fn encodedFrameLen(): usize {
        return 5 + _.encodedPayloadLen();
    }

    fn encodePayload(out: []u8): PgCopyError!usize {
        * = _.validate()?;
        const need = _.encodedPayloadLen();
        if (out.len < need) return PgCopyError.BufferTooSmall;
        let i: usize = 0;
        while (i < _.message.len) {
            out[i] = _.message[i];
            i += 1;
        }
        out[_.message.len] = 0;
        return need;
    }

    fn encodeFrame(out: []u8): PgCopyError!usize {
        * = _.validate()?;
        const payload_len = _.encodedPayloadLen();
        const total = 5 + payload_len;
        if (out.len < total) return PgCopyError.BufferTooSmall;
        const wire_len: u32 = @as(u32, @intCast(4 + payload_len));
        out[0] = PG_TAG_COPY_FAIL;
        writeBeU32(out, 1, wire_len);
        const written = _.encodePayload(out[5..])?;
        return 5 + written;
    }

    static fn decodePayload(payload: []const u8): PgCopyError!PgCopyFail {
        if (payload.len == 0) return PgCopyError.Truncated;
        let nul_pos: usize = 0;
        let found_nul = false;
        while (nul_pos < payload.len) {
            if (payload[nul_pos] == 0) {
                found_nul = true;
                break;
            }
            nul_pos += 1;
        }
        if (!found_nul) return PgCopyError.MissingNulTerminator;
        if (nul_pos + 1 != payload.len) return PgCopyError.TrailingBytes;
        return PgCopyFail{ message: payload[0..nul_pos] };
    }

    static fn decodeFrame(frame: []const u8): PgCopyError!PgCopyFail {
        if (frame.len < 5) return PgCopyError.Truncated;
        if (frame[0] != PG_TAG_COPY_FAIL) return PgCopyError.InvalidTag;
        const wire_len = readBeU32(frame, 1);
        if (wire_len < 5) return PgCopyError.InvalidLength;
        const total: usize = (@as(usize, wire_len)) + 1;
        if (frame.len < total) return PgCopyError.Truncated;
        if (frame.len > total) return PgCopyError.TrailingBytes;
        return PgCopyFail.decodePayload(frame[5..total]);
    }

    fn eql(other: PgCopyFail): bool {
        return bytesEql(_.message, other.message);
    }
}
