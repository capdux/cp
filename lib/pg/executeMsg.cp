// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Execute.cp | Synced: 2026-11-23T01:31:53Z
// ============================================================================
// lib/pg/executeMsg.cp — Zero-Alloc PostgreSQL Execute ('E') & Portal Control Codec
// ============================================================================
// Implements PostgreSQL Wire Protocol v3 `Execute` ('E') frontend message
// encoding and decoding (`[1B 'E'][4B BE len][portal\0][4B BE max_rows]`),
// `PortalTarget` (aligned with Bun `PortalOrPreparedStatement`), plus zero-alloc
// `Describe` ('D'), `Close` ('C'), `Sync` ('S') and `Flush` ('H') frame encoders.

pub const TAG_EXECUTE: u8 = 'E';
pub const TAG_DESCRIBE: u8 = 'D';
pub const TAG_CLOSE: u8 = 'C';
pub const TAG_SYNC: u8 = 'S';
pub const TAG_FLUSH: u8 = 'H';

pub const TARGET_PORTAL: u8 = 'P';
pub const TARGET_PREPARED_STATEMENT: u8 = 'S';
pub const EXECUTE_UNLIMITED_ROWS: i32 = 0;

pub err ExecuteMsgError {
    InvalidTag,
    InvalidLength,
    InvalidTargetByte,
    NegativeMaxRows,
    EmbeddedNul,
    Truncated,
    BufferTooSmall,
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readBeI32(buf: []const u8, off: usize): i32 {
    return @as(i32, @bitCast(readBeU32(buf, off)));
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
}

fn containsNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) return true;
        i += 1;
    }
    return false;
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

/// Identifies either a PostgreSQL Portal (`'P'`) or Prepared Statement (`'S'`),
/// aligned with Bun's `PortalOrPreparedStatement`.
pub struct PortalTarget {
    is_portal: bool = true,
    name: []const u8 = "",

    static fn unnamedPortal(): PortalTarget {
        return PortalTarget{ is_portal: true, name: "" };
    }

    static fn portal(name: []const u8): PortalTarget {
        return PortalTarget{ is_portal: true, name: name };
    }

    static fn preparedStatement(name: []const u8): PortalTarget {
        return PortalTarget{ is_portal: false, name: name };
    }

    fn targetByte(): u8 {
        return if (_.is_portal) TARGET_PORTAL else TARGET_PREPARED_STATEMENT;
    }

    /// Returns the portal name written into an `Execute` ('E') frame:
    /// named/unnamed portal name when `is_portal == true`, or empty string `""` otherwise.
    fn wirePortalName(): []const u8 {
        return if (_.is_portal) _.name else "";
    }

    fn isUnnamed(): bool {
        return _.name.len == 0;
    }

    fn eql(other: PortalTarget): bool {
        return _.is_portal == other.is_portal && bytesEql(_.name, other.name);
    }
}

/// PostgreSQL Extended Query `Execute` ('E') frontend message.
pub struct ExecuteMessage {
    target: PortalTarget = PortalTarget.unnamedPortal(),
    max_rows: i32 = EXECUTE_UNLIMITED_ROWS,

    static fn init(target: PortalTarget, max_rows: i32): ExecuteMessage {
        return ExecuteMessage{
            target: target,
            max_rows: max_rows,
        };
    }

    static fn unlimited(portal_name: []const u8): ExecuteMessage {
        return ExecuteMessage{
            target: PortalTarget.portal(portal_name),
            max_rows: EXECUTE_UNLIMITED_ROWS,
        };
    }

    fn isUnlimited(): bool {
        return _.max_rows == EXECUTE_UNLIMITED_ROWS;
    }

    fn encodedFrameLen(): usize {
        const portal_name = _.target.wirePortalName();
        return portal_name.len + 10;
    }

    /// Encodes `[1B 'E'][4B BE len][portal_name][0x00][4B BE max_rows]` into `out`.
    fn encodeFrame(out: []u8): ExecuteMsgError!usize {
        if (_.max_rows < 0) return ExecuteMsgError.NegativeMaxRows;
        const portal_name = _.target.wirePortalName();
        if (containsNul(portal_name)) return ExecuteMsgError.EmbeddedNul;

        const total = _.encodedFrameLen();
        if (out.len < total) return ExecuteMsgError.BufferTooSmall;

        const msg_len: u32 = @as(u32, @intCast(4 + portal_name.len + 1 + 4));
        out[0] = TAG_EXECUTE;
        writeBeU32(out, 1, msg_len);
        let i: usize = 0;
        while (i < portal_name.len) {
            out[5 + i] = portal_name[i];
            i += 1;
        }
        out[5 + portal_name.len] = 0;
        writeBeU32(out, 6 + portal_name.len, @as(u32, @bitCast(_.max_rows)));
        return total;
    }

    /// Decodes a complete `Execute` ('E') wire frame with zero allocation.
    static fn decodeFrame(frame: []const u8): ExecuteMsgError!ExecuteMessage {
        if (frame.len < 5) return ExecuteMsgError.Truncated;
        if (frame[0] != TAG_EXECUTE) return ExecuteMsgError.InvalidTag;
        const msg_len = readBeU32(frame, 1);
        if (msg_len < 9) return ExecuteMsgError.InvalidLength; // 4B len + 1B '\0' + 4B max_rows
        const total: usize = @as(usize, msg_len) + 1;
        if (frame.len < total) return ExecuteMsgError.Truncated;

        const payload = frame[5..total];
        const name_max = payload.len - 4;
        // Locate NUL terminator for portal name; must be exactly at `name_max - 1`
        let nul_pos: usize = name_max;
        let i: usize = 0;
        while (i < name_max) {
            if (payload[i] == 0) {
                nul_pos = i;
                break;
            }
            i += 1;
        }
        if (nul_pos != name_max - 1) return ExecuteMsgError.InvalidLength;

        const max_rows = readBeI32(payload, name_max);
        if (max_rows < 0) return ExecuteMsgError.NegativeMaxRows;

        return ExecuteMessage{
            target: PortalTarget.portal(payload[0..nul_pos]),
            max_rows: max_rows,
        };
    }

    fn eql(other: ExecuteMessage): bool {
        return _.target.eql(other.target) && _.max_rows == other.max_rows;
    }
}

/// Zero-alloc encoders for companion Extended Query control frames (`Describe`, `Close`, `Sync`, `Flush`).
pub struct ExecuteControlCodec {
    static fn encodeTargetCmd(tag: u8, target: PortalTarget, out: []u8): ExecuteMsgError!usize {
        if (containsNul(target.name)) return ExecuteMsgError.EmbeddedNul;
        const total: usize = target.name.len + 7;
        if (out.len < total) return ExecuteMsgError.BufferTooSmall;
        const msg_len: u32 = @as(u32, @intCast(target.name.len + 6));
        out[0] = tag;
        writeBeU32(out, 1, msg_len);
        out[5] = target.targetByte();
        let i: usize = 0;
        while (i < target.name.len) {
            out[6 + i] = target.name[i];
            i += 1;
        }
        out[6 + target.name.len] = 0;
        return total;
    }

    static fn encodeDescribeFrame(target: PortalTarget, out: []u8): ExecuteMsgError!usize {
        return ExecuteControlCodec.encodeTargetCmd(TAG_DESCRIBE, target, out);
    }

    static fn encodeCloseFrame(target: PortalTarget, out: []u8): ExecuteMsgError!usize {
        return ExecuteControlCodec.encodeTargetCmd(TAG_CLOSE, target, out);
    }

    static fn encodeSyncFrame(out: []u8): ExecuteMsgError!usize {
        if (out.len < 5) return ExecuteMsgError.BufferTooSmall;
        out[0] = TAG_SYNC;
        writeBeU32(out, 1, 4);
        return 5;
    }

    static fn encodeFlushFrame(out: []u8): ExecuteMsgError!usize {
        if (out.len < 5) return ExecuteMsgError.BufferTooSmall;
        out[0] = TAG_FLUSH;
        writeBeU32(out, 1, 4);
        return 5;
    }
}
