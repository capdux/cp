// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/LocalInfileRequest.cp | Synced: 2026-11-23T01:34:23Z
// ============================================================================
// lib/mysql/localInfile.cp — Zero-Alloc MySQL LOCAL INFILE Request (0xFB) Codec
// ============================================================================
// Implements MySQL Text Protocol `LOCAL INFILE Request` packet (first payload
// byte `0xFB`, followed by the server-requested client filename), plus empty
// terminator packet encoding and strict client-side path traversal policy guards.

pub const LOCAL_INFILE_HEADER: u8 = 0xFB;
pub const LOCAL_INFILE_MAX_FILENAME: usize = 4096;

pub err LocalInfileError {
    InvalidHeader,
    EmptyFilename,
    FilenameTooLong,
    EmbeddedNul,
    Truncated,
    TrailingBytes,
    BufferTooSmall,
    PathTraversalDenied,
    LocalFilesDisabled,
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

fn isSep(c: u8): bool {
    return c == '/' || c == '\\';
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn readLeU24(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    return b0 | (b1 << 8) | (b2 << 16);
}

fn writeLeU24(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast(v & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
}

/// MySQL `LOCAL INFILE Request` packet (`0xFB` + filename bytes).
pub struct LocalInfileRequest {
    filename: []const u8 = "",
    sequence_id: u8 = 0,

    static fn init(filename: []const u8, sequence_id: u8): LocalInfileRequest {
        return LocalInfileRequest{
            filename: filename,
            sequence_id: sequence_id,
        };
    }

    fn validate(): LocalInfileError!bool {
        if (_.filename.len == 0) return LocalInfileError.EmptyFilename;
        if (_.filename.len > LOCAL_INFILE_MAX_FILENAME) return LocalInfileError.FilenameTooLong;
        if (containsNul(_.filename)) return LocalInfileError.EmbeddedNul;
        return true;
    }

    fn isAbsolutePath(): bool {
        if (_.filename.len == 0) return false;
        if (isSep(_.filename[0])) return true;
        if (_.filename.len >= 2 && isAsciiAlpha(_.filename[0]) && _.filename[1] == ':') return true;
        return false;
    }

    /// Detects `..` path traversal segments across POSIX `/` and Windows `\` separators.
    fn hasPathTraversal(): bool {
        let i: usize = 0;
        let seg_start: usize = 0;
        while (i <= _.filename.len) {
            if (i == _.filename.len || isSep(_.filename[i])) {
                const seg_len = i - seg_start;
                if (seg_len == 2 && _.filename[seg_start] == '.' && _.filename[seg_start + 1] == '.') {
                    return true;
                }
                seg_start = i + 1;
            }
            i += 1;
        }
        return false;
    }

    fn isSafeRelativeFilename(): bool {
        if (_.filename.len == 0 || _.filename.len > LOCAL_INFILE_MAX_FILENAME) return false;
        if (containsNul(_.filename)) return false;
        if (_.isAbsolutePath()) return false;
        if (_.hasPathTraversal()) return false;
        return true;
    }

    /// Client-side security policy check before honoring a server `LOCAL INFILE` request.
    fn checkPolicy(allow_local_files: bool, allow_absolute: bool): LocalInfileError!bool {
        if (!allow_local_files) return LocalInfileError.LocalFilesDisabled;
        * = _.validate()?;
        if (_.hasPathTraversal()) return LocalInfileError.PathTraversalDenied;
        if (!allow_absolute && _.isAbsolutePath()) return LocalInfileError.PathTraversalDenied;
        return true;
    }

    fn encodedPayloadLen(): usize {
        return 1 + _.filename.len;
    }

    fn encodedFrameLen(): usize {
        return 4 + _.encodedPayloadLen();
    }

    /// Encodes the packet payload (`0xFB` + filename) into `out` without heap allocation.
    fn encodePayload(out: []u8): LocalInfileError!usize {
        * = _.validate()?;
        const need = _.encodedPayloadLen();
        if (out.len < need) return LocalInfileError.BufferTooSmall;
        out[0] = LOCAL_INFILE_HEADER;
        let i: usize = 0;
        while (i < _.filename.len) {
            out[1 + i] = _.filename[i];
            i += 1;
        }
        return need;
    }

    /// Encodes the full MySQL wire frame (3-byte LE length + 1-byte seq_id + payload).
    fn encodeFrame(out: []u8): LocalInfileError!usize {
        * = _.validate()?;
        const payload_len = _.encodedPayloadLen();
        const total = 4 + payload_len;
        if (out.len < total) return LocalInfileError.BufferTooSmall;
        writeLeU24(out, 0, @as(u32, @intCast(payload_len)));
        out[3] = _.sequence_id;
        const written = _.encodePayload(out[4..])?;
        return 4 + written;
    }

    /// Zero-copy decode of a `LOCAL INFILE Request` payload (borrows `filename` from `payload`).
    static fn decodePayload(payload: []const u8): LocalInfileError!LocalInfileRequest {
        if (payload.len == 0) return LocalInfileError.Truncated;
        if (payload[0] != LOCAL_INFILE_HEADER) return LocalInfileError.InvalidHeader;
        const fname = payload[1..];
        const req = LocalInfileRequest{
            filename: fname,
            sequence_id: 0,
        };
        * = req.validate()?;
        return req;
    }

    /// Zero-copy decode of a complete MySQL wire frame carrying `LOCAL INFILE Request`.
    static fn decodeFrame(frame: []const u8): LocalInfileError!LocalInfileRequest {
        if (frame.len < 4) return LocalInfileError.Truncated;
        const payload_len: usize = @as(usize, readLeU24(frame, 0));
        const seq_id = frame[3];
        const total: usize = payload_len + 4;
        if (frame.len < total) return LocalInfileError.Truncated;
        if (frame.len > total) return LocalInfileError.TrailingBytes;
        let req = LocalInfileRequest.decodePayload(frame[4..total])?;
        req.sequence_id = seq_id;
        return req;
    }

    /// Encodes the 4-byte empty packet that terminates client-to-server `LOCAL INFILE` data transfer.
    static fn encodeEmptyTerminatorFrame(sequence_id: u8, out: []u8): LocalInfileError!usize {
        if (out.len < 4) return LocalInfileError.BufferTooSmall;
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = sequence_id;
        return 4;
    }

    fn eql(other: LocalInfileRequest): bool {
        return _.sequence_id == other.sequence_id && bytesEql(_.filename, other.filename);
    }
}
