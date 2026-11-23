// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Close.cp | Synced: 2026-11-23T01:31:51Z
// ============================================================================
// lib/pgwire/closeDescribe.cp — Zero-Alloc PostgreSQL v3.0 Close ('C'),
// Describe ('D') & CloseComplete ('3') Extended Query Protocol Codec
// ============================================================================

pub const PG_TAG_CLOSE: u8 = 'C';
pub const PG_TAG_DESCRIBE: u8 = 'D';
pub const PG_TAG_CLOSE_COMPLETE: u8 = '3';
pub const PG_TARGET_PREPARED_STATEMENT: u8 = 'S';
pub const PG_TARGET_PORTAL: u8 = 'P';

pub err PgCloseDescribeError {
    InvalidTag,
    InvalidTargetKind,
    InvalidLength,
    Truncated,
    MissingNulTerminator,
    EmbeddedNul,
    TrailingBytes,
    BufferTooSmall,
}

pub enum PgTargetKind: u8 {
    PreparedStatement = 'S',
    Portal = 'P',

    fn tag(): u8 {
        return @intFromEnum(_);
    }

    static fn fromTag(b: u8): PgTargetKind? {
        if (b == PG_TARGET_PREPARED_STATEMENT) return .PreparedStatement;
        if (b == PG_TARGET_PORTAL) return .Portal;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .PreparedStatement => "prepared_statement",
            .Portal => "portal",
        };
    }
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

/// Identifies a PostgreSQL Extended Query target: Prepared Statement (`'S'`) or Portal (`'P'`).
/// An empty `name` refers to the unnamed statement or unnamed portal.
pub struct PgTargetRef {
    kind: PgTargetKind = .PreparedStatement,
    name: []const u8 = "",

    static fn preparedStatement(name: []const u8): PgTargetRef {
        return PgTargetRef{ kind: .PreparedStatement, name: name };
    }

    static fn portal(name: []const u8): PgTargetRef {
        return PgTargetRef{ kind: .Portal, name: name };
    }

    static fn unnamedStatement(): PgTargetRef {
        return PgTargetRef{ kind: .PreparedStatement, name: "" };
    }

    static fn unnamedPortal(): PgTargetRef {
        return PgTargetRef{ kind: .Portal, name: "" };
    }

    fn tag(): u8 {
        return _.kind.tag();
    }

    fn isUnnamed(): bool {
        return _.name.len == 0;
    }

    fn validate(): PgCloseDescribeError!bool {
        if (containsNul(_.name)) return PgCloseDescribeError.EmbeddedNul;
        return true;
    }

    fn encodedPayloadLen(): usize {
        return 1 + _.name.len + 1;
    }

    fn encodedFrameLen(): usize {
        return 1 + 4 + _.encodedPayloadLen();
    }

    fn encodePayload(out: []u8): PgCloseDescribeError!usize {
        * = _.validate()?;
        const need = _.encodedPayloadLen();
        if (out.len < need) return PgCloseDescribeError.BufferTooSmall;
        out[0] = _.tag();
        let i: usize = 0;
        while (i < _.name.len) {
            out[1 + i] = _.name[i];
            i += 1;
        }
        out[1 + _.name.len] = 0;
        return need;
    }

    static fn decodePayload(payload: []const u8): PgCloseDescribeError!PgTargetRef {
        if (payload.len < 2) return PgCloseDescribeError.Truncated;
        const kind = PgTargetKind.fromTag(payload[0]) ?? return PgCloseDescribeError.InvalidTargetKind;
        let nul_pos: usize = 1;
        let found_nul = false;
        while (nul_pos < payload.len) {
            if (payload[nul_pos] == 0) {
                found_nul = true;
                break;
            }
            nul_pos += 1;
        }
        if (!found_nul) return PgCloseDescribeError.MissingNulTerminator;
        if (nul_pos + 1 != payload.len) return PgCloseDescribeError.TrailingBytes;
        return PgTargetRef{
            kind: kind,
            name: payload[1..nul_pos],
        };
    }

    fn eql(other: PgTargetRef): bool {
        return _.kind == other.kind && bytesEql(_.name, other.name);
    }
}

fn encodeTaggedFrame(tag: u8, target: PgTargetRef, out: []u8): PgCloseDescribeError!usize {
    * = target.validate()?;
    const payload_len = target.encodedPayloadLen();
    const total = 5 + payload_len;
    if (out.len < total) return PgCloseDescribeError.BufferTooSmall;
    const wire_len: u32 = @as(u32, @intCast(4 + payload_len));
    out[0] = tag;
    writeBeU32(out, 1, wire_len);
    const written = target.encodePayload(out[5..])?;
    return 5 + written;
}

fn decodeTaggedFrame(expected_tag: u8, frame: []const u8): PgCloseDescribeError!PgTargetRef {
    if (frame.len < 5) return PgCloseDescribeError.Truncated;
    if (frame[0] != expected_tag) return PgCloseDescribeError.InvalidTag;
    const wire_len = readBeU32(frame, 1);
    if (wire_len < 6) return PgCloseDescribeError.InvalidLength;
    const total: usize = (@as(usize, wire_len)) + 1;
    if (frame.len < total) return PgCloseDescribeError.Truncated;
    if (frame.len > total) return PgCloseDescribeError.TrailingBytes;
    return PgTargetRef.decodePayload(frame[5..total]);
}

/// PostgreSQL Frontend `Close` message (Tag `'C'`).
pub struct PgCloseMsg {
    target: PgTargetRef = PgTargetRef{},

    static fn init(target: PgTargetRef): PgCloseMsg {
        return PgCloseMsg{ target: target };
    }

    static fn statement(name: []const u8): PgCloseMsg {
        return PgCloseMsg{ target: PgTargetRef.preparedStatement(name) };
    }

    static fn portal(name: []const u8): PgCloseMsg {
        return PgCloseMsg{ target: PgTargetRef.portal(name) };
    }

    fn encodedPayloadLen(): usize {
        return _.target.encodedPayloadLen();
    }

    fn encodedFrameLen(): usize {
        return _.target.encodedFrameLen();
    }

    fn encodePayload(out: []u8): PgCloseDescribeError!usize {
        return _.target.encodePayload(out);
    }

    fn encodeFrame(out: []u8): PgCloseDescribeError!usize {
        return encodeTaggedFrame(PG_TAG_CLOSE, _.target, out);
    }

    static fn decodePayload(payload: []const u8): PgCloseDescribeError!PgCloseMsg {
        const target = PgTargetRef.decodePayload(payload)?;
        return PgCloseMsg{ target: target };
    }

    static fn decodeFrame(frame: []const u8): PgCloseDescribeError!PgCloseMsg {
        const target = decodeTaggedFrame(PG_TAG_CLOSE, frame)?;
        return PgCloseMsg{ target: target };
    }

    /// Encodes Backend `CloseComplete` (`'3'`, length = 4) 5-byte frame.
    static fn encodeCloseCompleteFrame(out: []u8): PgCloseDescribeError!usize {
        if (out.len < 5) return PgCloseDescribeError.BufferTooSmall;
        out[0] = PG_TAG_CLOSE_COMPLETE;
        writeBeU32(out, 1, 4);
        return 5;
    }

    /// Validates Backend `CloseComplete` (`'3'`, length = 4) 5-byte frame.
    static fn decodeCloseCompleteFrame(frame: []const u8): PgCloseDescribeError!bool {
        if (frame.len < 5) return PgCloseDescribeError.Truncated;
        if (frame[0] != PG_TAG_CLOSE_COMPLETE) return PgCloseDescribeError.InvalidTag;
        const wire_len = readBeU32(frame, 1);
        if (wire_len != 4) return PgCloseDescribeError.InvalidLength;
        if (frame.len > 5) return PgCloseDescribeError.TrailingBytes;
        return true;
    }

    fn eql(other: PgCloseMsg): bool {
        return _.target.eql(other.target);
    }
}

/// PostgreSQL Frontend `Describe` message (Tag `'D'`).
pub struct PgDescribeMsg {
    target: PgTargetRef = PgTargetRef{},

    static fn init(target: PgTargetRef): PgDescribeMsg {
        return PgDescribeMsg{ target: target };
    }

    static fn statement(name: []const u8): PgDescribeMsg {
        return PgDescribeMsg{ target: PgTargetRef.preparedStatement(name) };
    }

    static fn portal(name: []const u8): PgDescribeMsg {
        return PgDescribeMsg{ target: PgTargetRef.portal(name) };
    }

    fn encodedPayloadLen(): usize {
        return _.target.encodedPayloadLen();
    }

    fn encodedFrameLen(): usize {
        return _.target.encodedFrameLen();
    }

    fn encodePayload(out: []u8): PgCloseDescribeError!usize {
        return _.target.encodePayload(out);
    }

    fn encodeFrame(out: []u8): PgCloseDescribeError!usize {
        return encodeTaggedFrame(PG_TAG_DESCRIBE, _.target, out);
    }

    static fn decodePayload(payload: []const u8): PgCloseDescribeError!PgDescribeMsg {
        const target = PgTargetRef.decodePayload(payload)?;
        return PgDescribeMsg{ target: target };
    }

    static fn decodeFrame(frame: []const u8): PgCloseDescribeError!PgDescribeMsg {
        const target = decodeTaggedFrame(PG_TAG_DESCRIBE, frame)?;
        return PgDescribeMsg{ target: target };
    }

    fn eql(other: PgDescribeMsg): bool {
        return _.target.eql(other.target);
    }
}
