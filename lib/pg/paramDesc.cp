// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/ParameterDescription.cp | Synced: 2026-11-23T01:32:19Z
// ============================================================================
// lib/pg/paramDesc.cp — Zero-Alloc PostgreSQL ParameterDescription ('t') Codec
// ============================================================================
// Implements PostgreSQL Wire Protocol v3 `ParameterDescription` ('t') backend
// message decoding and encoding: `[1B 't'][4B BE len][2B BE count][count * 4B BE oid]`.
// Provides both an unbounded zero-copy view (`ParameterDescriptionView`) and a
// fixed-capacity stack container (`ParameterDescription`, up to 256 parameters).

pub const TAG_PARAMETER_DESCRIPTION: u8 = 't';
pub const PARAM_DESC_MAX_PARAMS: usize = 256;
pub const PARAM_DESC_MIN_MSG_LEN: u32 = 6; // 4B length + 2B parameter count

pub err ParamDescError {
    InvalidTag,
    InvalidLength,
    NegativeCount,
    Truncated,
    TooManyParameters,
    BufferTooSmall,
}

fn readBeI16(buf: []const u8, off: usize): i16 {
    const hi: u16 = @as(u16, buf[off]);
    const lo: u16 = @as(u16, buf[off + 1]);
    return @as(i16, @bitCast((hi << 8) | lo));
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

/// Returns true if `oid` is a common PostgreSQL scalar OID well-suited for binary bind encoding.
pub fn isBinaryFriendlyParamOid(oid: u32): bool {
    return switch (oid) {
        16,   // BOOL
        17,   // BYTEA
        20,   // INT8
        21,   // INT2
        23,   // INT4
        25,   // TEXT
        700,  // FLOAT4
        701,  // FLOAT8
        1043, // VARCHAR
        1082, // DATE
        1114, // TIMESTAMP
        1184, // TIMESTAMPTZ
        2950, // UUID
        3802, // JSONB
        => true,
        else => false,
    };
}

/// Zero-copy, unbounded view over a `ParameterDescription` ('t') payload slice.
/// Reads big-endian 32-bit parameter OIDs directly from the wire buffer without allocation.
pub struct ParameterDescriptionView {
    payload: []const u8 = "",
    count: u16 = 0,

    /// Decodes a raw payload `[2B BE count][count * 4B BE oid]` (without the 5B tag+len header).
    static fn decodePayload(payload: []const u8): ParamDescError!ParameterDescriptionView {
        if (payload.len < 2) return ParamDescError.Truncated;
        const raw_count = readBeI16(payload, 0);
        if (raw_count < 0) return ParamDescError.NegativeCount;
        const count_u16: u16 = @as(u16, @intCast(raw_count));
        const expected_len: usize = @as(usize, count_u16) * 4 + 2;
        if (payload.len < expected_len) return ParamDescError.Truncated;
        if (payload.len != expected_len) return ParamDescError.InvalidLength;
        return ParameterDescriptionView{
            payload: payload,
            count: count_u16,
        };
    }

    /// Decodes a complete PostgreSQL wire frame `[1B 't'][4B BE len][payload]`.
    static fn decodeFrame(frame: []const u8): ParamDescError!ParameterDescriptionView {
        if (frame.len < 5) return ParamDescError.Truncated;
        if (frame[0] != TAG_PARAMETER_DESCRIPTION) return ParamDescError.InvalidTag;
        const msg_len = readBeU32(frame, 1);
        if (msg_len < PARAM_DESC_MIN_MSG_LEN) return ParamDescError.InvalidLength;
        const total_len: usize = @as(usize, msg_len) + 1;
        if (frame.len < total_len) return ParamDescError.Truncated;
        return ParameterDescriptionView.decodePayload(frame[5..total_len]);
    }

    /// Returns the big-endian parameter OID at `index`, or `null` if out of bounds.
    fn getOid(index: usize): u32? {
        if (index >= @as(usize, _.count)) return null;
        return readBeU32(_.payload, 2 + index * 4);
    }

    /// Returns true if any parameter has OID `0` (unspecified / server-inferred type).
    fn hasUnspecifiedTypes(): bool {
        let i: usize = 0;
        const n: usize = @as(usize, _.count);
        while (i < n) {
            if (readBeU32(_.payload, 2 + i * 4) == 0) return true;
            i += 1;
        }
        return false;
    }

    /// Returns true if every parameter OID supports direct binary parameter encoding.
    fn allBinaryFriendly(): bool {
        let i: usize = 0;
        const n: usize = @as(usize, _.count);
        while (i < n) {
            if (!isBinaryFriendlyParamOid(readBeU32(_.payload, 2 + i * 4))) return false;
            i += 1;
        }
        return true;
    }
}

/// Zero-alloc fixed-capacity `ParameterDescription` ('t') container (up to 256 parameters).
pub struct ParameterDescription {
    oids: [PARAM_DESC_MAX_PARAMS]u32 = [0; PARAM_DESC_MAX_PARAMS],
    len: usize = 0,

    static fn init(): ParameterDescription {
        return ParameterDescription{};
    }

    fn push(*_, oid: u32): ParamDescError!usize {
        if (_.len >= PARAM_DESC_MAX_PARAMS) return ParamDescError.TooManyParameters;
        _.oids[_.len] = oid;
        _.len += 1;
        return _.len;
    }

    fn get(index: usize): u32? {
        if (index >= _.len) return null;
        return _.oids[index];
    }

    fn slice(): []const u32 {
        return _.oids[0.._.len];
    }

    fn encodedFrameLen(): usize {
        return 1 + 4 + 2 + _.len * 4;
    }

    /// Encodes a complete PostgreSQL wire frame `[1B 't'][4B BE len][2B BE count][N * 4B BE oid]`.
    fn encodeFrame(out: []u8): ParamDescError!usize {
        const total = _.encodedFrameLen();
        if (out.len < total) return ParamDescError.BufferTooSmall;
        const msg_len: u32 = @as(u32, @intCast(4 + 2 + _.len * 4));
        out[0] = TAG_PARAMETER_DESCRIPTION;
        writeBeU32(out, 1, msg_len);
        writeBeU16(out, 5, @as(u16, @intCast(_.len)));
        let i: usize = 0;
        while (i < _.len) {
            writeBeU32(out, 7 + i * 4, _.oids[i]);
            i += 1;
        }
        return total;
    }

    static fn decodePayload(payload: []const u8): ParamDescError!ParameterDescription {
        const view = ParameterDescriptionView.decodePayload(payload)?;
        const n: usize = @as(usize, view.count);
        if (n > PARAM_DESC_MAX_PARAMS) return ParamDescError.TooManyParameters;
        let out = ParameterDescription.init();
        let i: usize = 0;
        while (i < n) {
            if (oid := view.getOid(i)) {
                out.oids[i] = oid;
            }
            i += 1;
        }
        out.len = n;
        return out;
    }

    static fn decodeFrame(frame: []const u8): ParamDescError!ParameterDescription {
        const view = ParameterDescriptionView.decodeFrame(frame)?;
        const n: usize = @as(usize, view.count);
        if (n > PARAM_DESC_MAX_PARAMS) return ParamDescError.TooManyParameters;
        let out = ParameterDescription.init();
        let i: usize = 0;
        while (i < n) {
            if (oid := view.getOid(i)) {
                out.oids[i] = oid;
            }
            i += 1;
        }
        out.len = n;
        return out;
    }

    fn eql(other: ParameterDescription): bool {
        if (_.len != other.len) return false;
        let i: usize = 0;
        while (i < _.len) {
            if (_.oids[i] != other.oids[i]) return false;
            i += 1;
        }
        return true;
    }
}
