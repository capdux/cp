// Source: Absorbed from /data/dev/bun/src/install/TarballStream.cp | Synced: 2026-11-23T20:22:40Z
// ============================================================================
// lib/pkg/tarballStream.cp — Zero-Alloc Streaming USTAR/PAX/GNU Tarball Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven incremental Tarball decoder & encoder
// for `cap install`: 512B header codec with unsigned/signed checksum verification,
// octal + GNU base-256 size decoding, USTAR prefix / GNU LongName(L)/LongLink(K) /
// POSIX PAX (x/g) extended headers, arbitrary chunk-size streaming state machine,
// strict Zip-Slip / Symlink-Escape / device-node / resource-exhaustion guards,
// and bidirectional USTAR/PAX archive encoder.

/// Standard TAR block size in bytes.
pub const TAR_BLOCK_SIZE: usize = 512;

/// Maximum sanitized relative entry path length.
pub const TAR_MAX_PATH_LEN: usize = 4096;

/// Default maximum entry count per archive (resource exhaustion guard).
pub const TAR_DEFAULT_MAX_FILES: u32 = 100000;

/// Default maximum single file size (256 MiB).
pub const TAR_DEFAULT_MAX_FILE_SIZE: u64 = 268435456;

/// Default maximum total uncompressed bytes (1 GiB).
pub const TAR_DEFAULT_MAX_TOTAL_BYTES: u64 = 1073741824;

/// Streaming tarball parsing, security validation, and encoding error set.
pub err TarStreamError {
    InvalidHeaderChecksum,
    InvalidOctalField,
    InvalidBase256Field,
    InvalidPaxRecord,
    UnsupportedEntryType,
    UnsafePathTraversal,
    UnsafeLinkTarget,
    DeviceNodeForbidden,
    MaxFilesExceeded,
    MaxFileSizeExceeded,
    MaxTotalSizeExceeded,
    UnexpectedEof,
    NameTooLongForUstar,
    BufferTooSmall,
    SliceCapacityExceeded,
    InvalidState,
}

/// TAR entry type classification derived from the 1-byte `typeflag` field.
pub enum TarEntryKind: u8 {
    RegularFile = 0,
    HardLink = 1,
    SymLink = 2,
    CharDevice = 3,
    BlockDevice = 4,
    Directory = 5,
    Fifo = 6,
    GnuLongName = 7,
    GnuLongLink = 8,
    PaxHeader = 9,
    PaxGlobalHeader = 10,
    Other = 11,
}

/// Security and stripping policy options for `TarStreamDecoder`.
pub struct TarStreamOptions {
    strip_components: u8 = 1,
    allow_symlinks: bool = true,
    allow_hardlinks: bool = true,
    allow_device_nodes: bool = false,
    max_files: u32 = TAR_DEFAULT_MAX_FILES,
    max_file_size: u64 = TAR_DEFAULT_MAX_FILE_SIZE,
    max_total_bytes: u64 = TAR_DEFAULT_MAX_TOTAL_BYTES,

    static fn defaults(): TarStreamOptions {
        return TarStreamOptions{};
    }

    static fn strictNpm(): TarStreamOptions {
        return TarStreamOptions{
            strip_components: 1,
            allow_symlinks: true,
            allow_hardlinks: false,
            allow_device_nodes: false,
            max_files: TAR_DEFAULT_MAX_FILES,
            max_file_size: TAR_DEFAULT_MAX_FILE_SIZE,
            max_total_bytes: TAR_DEFAULT_MAX_TOTAL_BYTES,
        };
    }
}

/// Decoded raw fields from a single 512-byte TAR header block.
pub struct TarParsedHeader {
    kind: TarEntryKind = TarEntryKind.RegularFile,
    raw_typeflag: u8 = 0,
    mode: u32 = 0,
    uid: u32 = 0,
    gid: u32 = 0,
    size: u64 = 0,
    mtime: u64 = 0,
    checksum: u32 = 0,
    is_ustar: bool = false,
    raw_name: []const u8 = "",
    raw_prefix: []const u8 = "",
    raw_linkname: []const u8 = "",
    uname: []const u8 = "",
    gname: []const u8 = "",
}

/// Event kind produced by `TarStreamDecoder.step`.
pub enum TarStreamEventKind: u8 {
    NeedMoreData = 0,
    EntryHeader = 1,
    EntryDataChunk = 2,
    EntryEnd = 3,
    ArchiveEnd = 4,
}

/// Sanitized, resolved metadata for a regular file, directory, or link entry.
pub struct TarStreamEntryMeta {
    kind: TarEntryKind = TarEntryKind.RegularFile,
    path: []const u8 = "",
    link_target: []const u8 = "",
    mode: u32 = 0,
    size: u64 = 0,
    mtime: u64 = 0,
    stripped_skipped: bool = false,

    static fn empty(): TarStreamEntryMeta {
        return TarStreamEntryMeta{};
    }
}

/// Single step result from `TarStreamDecoder.step`.
pub struct TarStreamEvent {
    kind: TarStreamEventKind = TarStreamEventKind.NeedMoreData,
    consumed: usize = 0,
    entry: TarStreamEntryMeta = TarStreamEntryMeta.empty(),
    data: []const u8 = "",
}

/// Cumulative statistics tracked by `TarStreamDecoder`.
pub struct TarStreamStats {
    entries_seen: u32 = 0,
    files_extracted: u32 = 0,
    dirs_extracted: u32 = 0,
    links_extracted: u32 = 0,
    skipped_entries: u32 = 0,
    total_data_bytes: u64 = 0,
    total_stream_bytes: u64 = 0,
    pax_headers_applied: u32 = 0,
    gnu_long_headers_applied: u32 = 0,

    static fn empty(): TarStreamStats {
        return TarStreamStats{};
    }
}

/// Internal phase of `TarStreamDecoder`.
pub enum TarStreamPhase: u8 {
    ReadHeader = 0,
    ReadMetaPayload = 1,
    ReadEntryData = 2,
    SkipPadding = 3,
    Done = 4,
}

// ============================================================================
// Internal Byte & String Utilities (Zero-Alloc)
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn cstrSlice(field: []const u8): []const u8 {
    let i: usize = 0;
    while (i < field.len && field[i] != 0) {
        i += 1;
    }
    return field[0..i];
}

fn trimNullAndSpace(field: []const u8): []const u8 {
    let start: usize = 0;
    while (start < field.len && (field[start] == ' ' || field[start] == 0)) {
        start += 1;
    }
    let end: usize = field.len;
    while (end > start && (field[end - 1] == ' ' || field[end - 1] == 0)) {
        end -= 1;
    }
    return field[start..end];
}

fn copyBytes(dst: []u8, src: []const u8)! {
    if (src.len > dst.len) return TarStreamError.BufferTooSmall;
    if (src.len > 0) {
        @memcpy(dst[0..src.len], src);
    }
}

fn padTo512(size: u64): usize {
    const rem: usize = @intCast(size & 511);
    if (rem == 0) return 0;
    return 512 - rem;
}

/// Returns true if all 512 bytes of `block` are zero (TAR EOF marker block).
pub fn isZeroTarBlock(block: []const u8): bool {
    if (block.len < TAR_BLOCK_SIZE) return false;
    let i: usize = 0;
    while (i < TAR_BLOCK_SIZE) {
        if (block[i] != 0) return false;
        i += 1;
    }
    return true;
}

/// Parses a TAR numeric field encoded as either ASCII octal or GNU base-256 binary.
pub fn parseTarOctalOrBase256(field: []const u8): u64! {
    if (field.len == 0) return 0;

    // GNU base-256: high bit 0x80 set on first byte (positive big-endian integer)
    if ((field[0] & 0x80) != 0) {
        // Negative base-256 (0xFF) is invalid for unsigned sizes/modes
        if ((field[0] & 0x40) != 0) {
            return TarStreamError.InvalidBase256Field;
        }
        let val: u64 = @as(u64, field[0] & 0x7F);
        let i: usize = 1;
        while (i < field.len) {
            if (val > (18446744073709551615 >> 8)) {
                return TarStreamError.InvalidBase256Field;
            }
            val = (val << 8) | @as(u64, field[i]);
            i += 1;
        }
        return val;
    }

    const s = trimNullAndSpace(field);
    if (s.len == 0) return 0;

    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == 0 || c == ' ') break;
        if (c < '0' || c > '7') {
            return TarStreamError.InvalidOctalField;
        }
        val = (val << 3) | @as(u64, c - '0');
        i += 1;
    }
    return val;
}

/// Verifies a 512-byte TAR header block checksum against both unsigned and signed sums.
pub fn verifyTarHeaderChecksum(block: []const u8): bool {
    if (block.len < TAR_BLOCK_SIZE) return false;
    const expected_u64 = parseTarOctalOrBase256(block[148..156]) catch return false;
    if (expected_u64 > 4294967295) return false;
    const expected: i64 = @intCast(expected_u64);

    let unsigned_sum: i64 = 0;
    let signed_sum: i64 = 0;
    let i: usize = 0;
    while (i < TAR_BLOCK_SIZE) {
        const b: u8 = if (i >= 148 && i < 156) @as(u8, 32) else block[i];
        unsigned_sum += @as(i64, b);
        const sb: i64 = if (b >= 128) @as(i64, b) - 256 else @as(i64, b);
        signed_sum += sb;
        i += 1;
    }
    return (unsigned_sum == expected) || (signed_sum == expected);
}

fn classifyTypeflag(tf: u8): TarEntryKind {
    if (tf == 0 || tf == '0') return TarEntryKind.RegularFile;
    if (tf == '1') return TarEntryKind.HardLink;
    if (tf == '2') return TarEntryKind.SymLink;
    if (tf == '3') return TarEntryKind.CharDevice;
    if (tf == '4') return TarEntryKind.BlockDevice;
    if (tf == '5') return TarEntryKind.Directory;
    if (tf == '6') return TarEntryKind.Fifo;
    if (tf == 'L') return TarEntryKind.GnuLongName;
    if (tf == 'K') return TarEntryKind.GnuLongLink;
    if (tf == 'x') return TarEntryKind.PaxHeader;
    if (tf == 'g') return TarEntryKind.PaxGlobalHeader;
    return TarEntryKind.Other;
}

/// Parses and checksum-validates a single 512-byte TAR header block.
pub fn parseTarHeaderBlock(block: []const u8): TarParsedHeader! {
    if (block.len < TAR_BLOCK_SIZE) {
        return TarStreamError.UnexpectedEof;
    }
    if (!verifyTarHeaderChecksum(block)) {
        return TarStreamError.InvalidHeaderChecksum;
    }

    const mode_u64 = parseTarOctalOrBase256(block[100..108])?;
    const uid_u64 = parseTarOctalOrBase256(block[108..116])?;
    const gid_u64 = parseTarOctalOrBase256(block[116..124])?;
    const size = parseTarOctalOrBase256(block[124..136])?;
    const mtime = parseTarOctalOrBase256(block[136..148])?;
    const chksum_u64 = parseTarOctalOrBase256(block[148..156])?;

    const tf = block[156];
    const magic = block[257..263];
    const is_ustar = bytesEqual(magic, "ustar\x00") || bytesEqual(magic, "ustar ");

    return TarParsedHeader{
        kind: classifyTypeflag(tf),
        raw_typeflag: tf,
        mode: @intCast(mode_u64 & 0xFFFF),
        uid: @intCast(uid_u64 & 0xFFFFFFFF),
        gid: @intCast(gid_u64 & 0xFFFFFFFF),
        size: size,
        mtime: mtime,
        checksum: @intCast(chksum_u64 & 0xFFFFFFFF),
        is_ustar: is_ustar,
        raw_name: cstrSlice(block[0..100]),
        raw_prefix: if (is_ustar) cstrSlice(block[345..500]) else "",
        raw_linkname: cstrSlice(block[157..257]),
        uname: if (is_ustar) cstrSlice(block[265..297]) else "",
        gname: if (is_ustar) cstrSlice(block[297..329]) else "",
    };
}

// ============================================================================
// Path Traversal & Symlink Escape Security Guards
// ============================================================================

/// Sanitizes a raw TAR entry path, applies `strip_components`, and writes the safe
/// relative path into `out_buf`. Returns `0` if the entry was stripped away cleanly
/// (e.g. top-level `package/` directory with `strip_components = 1`).
/// Strictly rejects absolute paths, backslashes, Windows drive prefixes, NUL bytes,
/// and any `..` traversal segment.
pub fn sanitizeTarEntryPath(
    raw_path: []const u8,
    strip_components: u8,
    out_buf: []u8,
): usize! {
    if (raw_path.len == 0 || raw_path.len > TAR_MAX_PATH_LEN) {
        return TarStreamError.UnsafePathTraversal;
    }
    if (raw_path[0] == '/' || raw_path[0] == '\\') {
        return TarStreamError.UnsafePathTraversal;
    }
    if (raw_path.len >= 2 && raw_path[1] == ':') {
        const c0 = raw_path[0];
        if ((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z')) {
            return TarStreamError.UnsafePathTraversal;
        }
    }

    let out_pos: usize = 0;
    let stripped: u8 = 0;
    let seg_start: usize = 0;
    let i: usize = 0;

    while (i <= raw_path.len) {
        const at_end = (i == raw_path.len);
        if (!at_end) {
            const c = raw_path[i];
            if (c == 0 || c == '\\' || c == '\r' || c == '\n') {
                return TarStreamError.UnsafePathTraversal;
            }
        }
        if (at_end || raw_path[i] == '/') {
            const seg = raw_path[seg_start..i];
            if (seg.len > 0) {
                if (bytesEqual(seg, "..")) {
                    return TarStreamError.UnsafePathTraversal;
                }
                if (!bytesEqual(seg, ".")) {
                    if (stripped < strip_components) {
                        stripped += 1;
                    } else {
                        if (out_pos > 0) {
                            if (out_pos >= out_buf.len) return TarStreamError.BufferTooSmall;
                            out_buf[out_pos] = @as(u8, 47); // '/'
                            out_pos += 1;
                        }
                        if (out_pos + seg.len > out_buf.len) return TarStreamError.BufferTooSmall;
                        let k: usize = 0;
                        while (k < seg.len) {
                            out_buf[out_pos + k] = seg[k];
                            k += 1;
                        }
                        out_pos += seg.len;
                    }
                }
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return out_pos;
}

/// Verifies that a symlink or hardlink target cannot escape the extraction root directory.
/// - Hardlinks are resolved relative to extraction root (must not contain `..` or absolute paths).
/// - Symlinks are resolved relative to the parent directory of `entry_path`; tracks depth
///   across every segment so `..` can never drop below root depth 0.
pub fn validateTarLinkTarget(
    entry_path: []const u8,
    link_target: []const u8,
    is_symlink: bool,
): bool {
    if (link_target.len == 0 || link_target.len > TAR_MAX_PATH_LEN) return false;
    if (link_target[0] == '/' || link_target[0] == '\\') return false;
    if (link_target.len >= 2 && link_target[1] == ':') {
        const c0 = link_target[0];
        if ((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z')) return false;
    }

    // Compute starting directory depth of entry_path (number of '/' separators)
    let depth: i32 = 0;
    if (is_symlink) {
        let p: usize = 0;
        while (p < entry_path.len) {
            if (entry_path[p] == '/') depth += 1;
            p += 1;
        }
    }

    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= link_target.len) {
        const at_end = (i == link_target.len);
        if (!at_end) {
            const c = link_target[i];
            if (c == 0 || c == '\\' || c == '\r' || c == '\n') return false;
        }
        if (at_end || link_target[i] == '/') {
            const seg = link_target[seg_start..i];
            if (seg.len > 0 && !bytesEqual(seg, ".")) {
                if (bytesEqual(seg, "..")) {
                    if (!is_symlink) return false;
                    depth -= 1;
                    if (depth < 0) return false;
                } else {
                    depth += 1;
                }
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return true;
}

// ============================================================================
// POSIX PAX Extended Header Record Parser (`<len> <key>=<value>\n`)
// ============================================================================

fn parseDecimalU64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u64, c - '0');
        i += 1;
    }
    return val;
}

/// Output struct populated by `parsePaxRecords`.
pub struct TarPaxOverrides {
    path_len: usize = 0,
    link_len: usize = 0,
    size_override: u64? = null,
    mtime_override: u64? = null,
}

/// Parses POSIX PAX extended header records (`<len> <key>=<value>\n`), extracting
/// `path`, `linkpath`, `size`, and `mtime` overrides into caller buffers.
pub fn parsePaxRecords(
    payload: []const u8,
    path_buf: []u8,
    link_buf: []u8,
    out: *TarPaxOverrides,
)! {
    let pos: usize = 0;
    while (pos < payload.len) {
        // Parse decimal length up to space
        let sp = pos;
        while (sp < payload.len && payload[sp] != ' ') {
            sp += 1;
        }
        if (sp == pos || sp >= payload.len) {
            return TarStreamError.InvalidPaxRecord;
        }
        const rec_len_opt = parseDecimalU64(payload[pos..sp]);
        if (rec_len_opt == null) return TarStreamError.InvalidPaxRecord;
        const rec_len: usize = @intCast(rec_len_opt.?);
        if (rec_len < (sp - pos) + 3 || pos + rec_len > payload.len) {
            return TarStreamError.InvalidPaxRecord;
        }
        const rec_end = pos + rec_len;
        if (payload[rec_end - 1] != '\n') {
            return TarStreamError.InvalidPaxRecord;
        }
        const kv = payload[sp + 1 .. rec_end - 1];
        let eq: usize = 0;
        while (eq < kv.len && kv[eq] != '=') {
            eq += 1;
        }
        if (eq == 0 || eq >= kv.len) {
            return TarStreamError.InvalidPaxRecord;
        }
        const key = kv[0..eq];
        const val = kv[eq + 1 ..];

        if (bytesEqual(key, "path")) {
            copyBytes(path_buf, val)?;
            out.path_len = val.len;
        } else if (bytesEqual(key, "linkpath")) {
            copyBytes(link_buf, val)?;
            out.link_len = val.len;
        } else if (bytesEqual(key, "size")) {
            const sz_opt = parseDecimalU64(val);
            if (sz_opt == null) return TarStreamError.InvalidPaxRecord;
            out.size_override = sz_opt.?;
        } else if (bytesEqual(key, "mtime")) {
            // PAX mtime may include fractional seconds like `1700000000.123`; take integer part
            let dot: usize = 0;
            while (dot < val.len && val[dot] != '.') {
                dot += 1;
            }
            if (mt := parseDecimalU64(val[0..dot])) {
                out.mtime_override = mt;
            }
        }
        pos = rec_end;
    }
}

// ============================================================================
// Incremental Streaming Tarball Decoder (`TarStreamDecoder`)
// ============================================================================

/// Zero-heap incremental Tarball streaming decoder.
/// Feeds arbitrary chunk sizes (e.g., 17B, 1KB, 64KB) via `step()` and emits
/// `EntryHeader`, `EntryDataChunk`, `EntryEnd`, `NeedMoreData`, and `ArchiveEnd`.
pub struct TarStreamDecoder {
    opts: TarStreamOptions = TarStreamOptions.defaults(),
    phase: TarStreamPhase = TarStreamPhase.ReadHeader,
    hdr_buf: [512]u8 = undefined,
    hdr_filled: usize = 0,
    zero_blocks_seen: u8 = 0,

    // Caller-owned scratch buffers (zero heap)
    path_buf: []u8,
    link_buf: []u8,
    meta_buf: []u8,
    meta_len: usize = 0,
    meta_remaining: usize = 0,
    meta_kind: TarEntryKind = TarEntryKind.Other,

    // Pending overrides from GNU L/K or PAX x/g extended headers
    pending_path_len: usize = 0,
    pending_link_len: usize = 0,
    pending_size: u64? = null,
    pending_mtime: u64? = null,
    global_mtime: u64? = null,

    // Active entry streaming cursors
    cur_entry: TarStreamEntryMeta = TarStreamEntryMeta.empty(),
    data_remaining: u64 = 0,
    padding_remaining: usize = 0,
    stats_val: TarStreamStats = TarStreamStats.empty(),

    static fn init(
        opts: TarStreamOptions,
        path_buf: []u8,
        link_buf: []u8,
        meta_buf: []u8,
    ): TarStreamDecoder {
        return TarStreamDecoder{
            opts: opts,
            path_buf: path_buf,
            link_buf: link_buf,
            meta_buf: meta_buf,
        };
    }

    fn stats(): TarStreamStats {
        return _.stats_val;
    }

    fn isDone(): bool {
        return _.phase == TarStreamPhase.Done;
    }

    /// Verifies clean stream termination (rejects truncated headers or entry payloads).
    fn finish(*_)! {
        if (_.phase == TarStreamPhase.Done) return;
        if (_.phase == TarStreamPhase.ReadHeader && _.hdr_filled == 0 && _.zero_blocks_seen > 0) {
            _.phase = TarStreamPhase.Done;
            return;
        }
        return TarStreamError.UnexpectedEof;
    }

    fn clearPendingOverrides(*_) {
        _.pending_path_len = 0;
        _.pending_link_len = 0;
        _.pending_size = null;
        _.pending_mtime = null;
    }

    /// Advances the streaming decoder by consuming bytes from `chunk`.
    /// Callers loop while `consumed < chunk.len` until `NeedMoreData` or `ArchiveEnd`.
    fn step(*_, chunk: []const u8): TarStreamEvent! {
        let off: usize = 0;

        while (true) {
            if (_.phase == TarStreamPhase.Done) {
                return TarStreamEvent{
                    kind: TarStreamEventKind.ArchiveEnd,
                    consumed: off,
                };
            }

            if (_.phase == TarStreamPhase.SkipPadding) {
                if (_.padding_remaining == 0) {
                    _.phase = TarStreamPhase.ReadHeader;
                    continue;
                }
                if (off >= chunk.len) {
                    return TarStreamEvent{
                        kind: TarStreamEventKind.NeedMoreData,
                        consumed: off,
                    };
                }
                const avail = chunk.len - off;
                const take = if (avail < _.padding_remaining) avail else _.padding_remaining;
                off += take;
                _.padding_remaining -= take;
                _.stats_val.total_stream_bytes += @as(u64, take);
                if (_.padding_remaining == 0) {
                    _.phase = TarStreamPhase.ReadHeader;
                }
                continue;
            }

            if (_.phase == TarStreamPhase.ReadMetaPayload) {
                if (_.meta_remaining > 0) {
                    if (off >= chunk.len) {
                        return TarStreamEvent{
                            kind: TarStreamEventKind.NeedMoreData,
                            consumed: off,
                        };
                    }
                    const avail = chunk.len - off;
                    const take = if (avail < _.meta_remaining) avail else _.meta_remaining;
                    if (_.meta_len + take > _.meta_buf.len) {
                        return TarStreamError.BufferTooSmall;
                    }
                    let i: usize = 0;
                    while (i < take) {
                        _.meta_buf[_.meta_len + i] = chunk[off + i];
                        i += 1;
                    }
                    _.meta_len += take;
                    _.meta_remaining -= take;
                    off += take;
                    _.stats_val.total_stream_bytes += @as(u64, take);
                    if (_.meta_remaining > 0) {
                        continue;
                    }
                }

                // Meta payload fully buffered; apply override
                const payload = _.meta_buf[0.._.meta_len];
                if (_.meta_kind == TarEntryKind.GnuLongName) {
                    const clean = cstrSlice(payload);
                    copyBytes(_.path_buf, clean)?;
                    _.pending_path_len = clean.len;
                    _.stats_val.gnu_long_headers_applied += 1;
                } else if (_.meta_kind == TarEntryKind.GnuLongLink) {
                    const clean = cstrSlice(payload);
                    copyBytes(_.link_buf, clean)?;
                    _.pending_link_len = clean.len;
                    _.stats_val.gnu_long_headers_applied += 1;
                } else if (_.meta_kind == TarEntryKind.PaxHeader) {
                    let pax_ov = TarPaxOverrides{
                        path_len: _.pending_path_len,
                        link_len: _.pending_link_len,
                        size_override: _.pending_size,
                        mtime_override: _.pending_mtime,
                    };
                    parsePaxRecords(payload, _.path_buf, _.link_buf, &pax_ov)?;
                    _.pending_path_len = pax_ov.path_len;
                    _.pending_link_len = pax_ov.link_len;
                    _.pending_size = pax_ov.size_override;
                    _.pending_mtime = pax_ov.mtime_override;
                    _.stats_val.pax_headers_applied += 1;
                } else if (_.meta_kind == TarEntryKind.PaxGlobalHeader) {
                    let pax_ov = TarPaxOverrides{
                        mtime_override: _.global_mtime,
                    };
                    let dummy_path: [1]u8 = [0; 1];
                    let dummy_link: [1]u8 = [0; 1];
                    parsePaxRecords(payload, dummy_path[0..], dummy_link[0..], &pax_ov)?;
                    _.global_mtime = pax_ov.mtime_override;
                    _.stats_val.pax_headers_applied += 1;
                }

                _.meta_len = 0;
                _.phase = TarStreamPhase.SkipPadding;
                continue;
            }

            if (_.phase == TarStreamPhase.ReadEntryData) {
                if (_.data_remaining == 0) {
                    _.phase = TarStreamPhase.SkipPadding;
                    return TarStreamEvent{
                        kind: TarStreamEventKind.EntryEnd,
                        consumed: off,
                        entry: _.cur_entry,
                    };
                }
                if (off >= chunk.len) {
                    return TarStreamEvent{
                        kind: TarStreamEventKind.NeedMoreData,
                        consumed: off,
                    };
                }
                const avail: u64 = @as(u64, chunk.len - off);
                const take_u64 = if (avail < _.data_remaining) avail else _.data_remaining;
                const take: usize = @intCast(take_u64);
                const slice = chunk[off .. off + take];
                off += take;
                _.data_remaining -= take_u64;
                _.stats_val.total_data_bytes += take_u64;
                _.stats_val.total_stream_bytes += take_u64;

                if (_.cur_entry.stripped_skipped) {
                    // Discard payload for stripped entries
                    continue;
                }

                return TarStreamEvent{
                    kind: TarStreamEventKind.EntryDataChunk,
                    consumed: off,
                    entry: _.cur_entry,
                    data: slice,
                };
            }

            // Phase == ReadHeader: accumulate 512 bytes
            if (_.hdr_filled < TAR_BLOCK_SIZE) {
                if (off >= chunk.len) {
                    return TarStreamEvent{
                        kind: TarStreamEventKind.NeedMoreData,
                        consumed: off,
                    };
                }
                const need = TAR_BLOCK_SIZE - _.hdr_filled;
                const avail = chunk.len - off;
                const take = if (avail < need) avail else need;
                let i: usize = 0;
                while (i < take) {
                    _.hdr_buf[_.hdr_filled + i] = chunk[off + i];
                    i += 1;
                }
                _.hdr_filled += take;
                off += take;
                _.stats_val.total_stream_bytes += @as(u64, take);
                if (_.hdr_filled < TAR_BLOCK_SIZE) {
                    continue;
                }
            }

            _.hdr_filled = 0;

            // Check EOF zero block
            if (isZeroTarBlock(_.hdr_buf[0..])) {
                _.zero_blocks_seen += 1;
                if (_.zero_blocks_seen >= 2) {
                    _.phase = TarStreamPhase.Done;
                    return TarStreamEvent{
                        kind: TarStreamEventKind.ArchiveEnd,
                        consumed: off,
                    };
                }
                continue;
            }
            _.zero_blocks_seen = 0;

            const hdr = parseTarHeaderBlock(_.hdr_buf[0..])?;

            // Handle metadata pseudo-entries (GNU L/K, PAX x/g)
            if (hdr.kind == TarEntryKind.GnuLongName ||
                hdr.kind == TarEntryKind.GnuLongLink ||
                hdr.kind == TarEntryKind.PaxHeader ||
                hdr.kind == TarEntryKind.PaxGlobalHeader)
            {
                if (hdr.size > @as(u64, _.meta_buf.len)) {
                    return TarStreamError.BufferTooSmall;
                }
                _.meta_kind = hdr.kind;
                _.meta_len = 0;
                _.meta_remaining = @intCast(hdr.size);
                _.padding_remaining = padTo512(hdr.size);
                _.phase = TarStreamPhase.ReadMetaPayload;
                continue;
            }

            // Device node guard
            if (hdr.kind == TarEntryKind.CharDevice ||
                hdr.kind == TarEntryKind.BlockDevice ||
                hdr.kind == TarEntryKind.Fifo)
            {
                if (!_.opts.allow_device_nodes) {
                    return TarStreamError.DeviceNodeForbidden;
                }
            }

            // Resource limit guards
            _.stats_val.entries_seen += 1;
            if (_.stats_val.entries_seen > _.opts.max_files) {
                return TarStreamError.MaxFilesExceeded;
            }

            const eff_size = _.pending_size ?? hdr.size;
            const eff_mtime = _.pending_mtime ?? (_.global_mtime ?? hdr.mtime);
            if (eff_size > _.opts.max_file_size) {
                return TarStreamError.MaxFileSizeExceeded;
            }
            if (_.stats_val.total_data_bytes + eff_size > _.opts.max_total_bytes) {
                return TarStreamError.MaxTotalSizeExceeded;
            }

            // Assemble raw path (PAX/GNU LongName > USTAR prefix+name > name)
            let raw_path_tmp: [4096]u8 = undefined;
            let raw_path_len: usize = 0;
            if (_.pending_path_len > 0) {
                copyBytes(raw_path_tmp[0..], _.path_buf[0.._.pending_path_len])?;
                raw_path_len = _.pending_path_len;
            } else if (hdr.raw_prefix.len > 0) {
                copyBytes(raw_path_tmp[0..], hdr.raw_prefix)?;
                raw_path_len = hdr.raw_prefix.len;
                if (raw_path_len >= raw_path_tmp.len) return TarStreamError.BufferTooSmall;
                raw_path_tmp[raw_path_len] = @as(u8, 47); // '/'
                raw_path_len += 1;
                copyBytes(raw_path_tmp[raw_path_len..], hdr.raw_name)?;
                raw_path_len += hdr.raw_name.len;
            } else {
                copyBytes(raw_path_tmp[0..], hdr.raw_name)?;
                raw_path_len = hdr.raw_name.len;
            }

            // Sanitize path with strip_components and Zip-Slip guard
            const clean_path_len = sanitizeTarEntryPath(
                raw_path_tmp[0..raw_path_len],
                _.opts.strip_components,
                _.path_buf,
            )?;

            // Resolve and validate link target if symlink/hardlink
            let clean_link_len: usize = 0;
            if (hdr.kind == TarEntryKind.SymLink || hdr.kind == TarEntryKind.HardLink) {
                if (hdr.kind == TarEntryKind.SymLink && !_.opts.allow_symlinks) {
                    return TarStreamError.UnsafeLinkTarget;
                }
                if (hdr.kind == TarEntryKind.HardLink && !_.opts.allow_hardlinks) {
                    return TarStreamError.UnsafeLinkTarget;
                }
                if (_.pending_link_len > 0) {
                    clean_link_len = _.pending_link_len;
                } else {
                    copyBytes(_.link_buf, hdr.raw_linkname)?;
                    clean_link_len = hdr.raw_linkname.len;
                }
                if (clean_path_len > 0) {
                    const is_sym = (hdr.kind == TarEntryKind.SymLink);
                    if (!validateTarLinkTarget(_.path_buf[0..clean_path_len], _.link_buf[0..clean_link_len], is_sym)) {
                        return TarStreamError.UnsafeLinkTarget;
                    }
                }
            }

            _.clearPendingOverrides();

            const stripped_skipped = (clean_path_len == 0);
            _.cur_entry = TarStreamEntryMeta{
                kind: hdr.kind,
                path: _.path_buf[0..clean_path_len],
                link_target: _.link_buf[0..clean_link_len],
                mode: hdr.mode,
                size: eff_size,
                mtime: eff_mtime,
                stripped_skipped: stripped_skipped,
            };
            _.data_remaining = eff_size;
            _.padding_remaining = padTo512(eff_size);
            _.phase = TarStreamPhase.ReadEntryData;

            if (stripped_skipped) {
                _.stats_val.skipped_entries += 1;
                continue;
            }

            if (hdr.kind == TarEntryKind.RegularFile) {
                _.stats_val.files_extracted += 1;
            } else if (hdr.kind == TarEntryKind.Directory) {
                _.stats_val.dirs_extracted += 1;
            } else if (hdr.kind == TarEntryKind.SymLink || hdr.kind == TarEntryKind.HardLink) {
                _.stats_val.links_extracted += 1;
            }

            return TarStreamEvent{
                kind: TarStreamEventKind.EntryHeader,
                consumed: off,
                entry: _.cur_entry,
            };
        }
    }
}

// ============================================================================
// USTAR / PAX Tarball Encoder (Bidirectional Self-Test & Pack Support)
// ============================================================================

fn writeOctalField(dst: []u8, value: u64) {
    // Fill with '0', terminate last byte with NUL
    let i: usize = 0;
    while (i < dst.len) {
        dst[i] = @as(u8, 48); // '0'
        i += 1;
    }
    if (dst.len == 0) return;
    dst[dst.len - 1] = 0;
    let v = value;
    let idx: usize = dst.len - 1;
    while (idx > 0 && v > 0) {
        idx -= 1;
        dst[idx] = @intCast(48 + (v & 7));
        v = v >> 3;
    }
}

fn kindToTypeflag(kind: TarEntryKind): u8 {
    if (kind == TarEntryKind.RegularFile) return @as(u8, 48); // '0'
    if (kind == TarEntryKind.HardLink) return @as(u8, 49); // '1'
    if (kind == TarEntryKind.SymLink) return @as(u8, 50); // '2'
    if (kind == TarEntryKind.CharDevice) return @as(u8, 51); // '3'
    if (kind == TarEntryKind.BlockDevice) return @as(u8, 52); // '4'
    if (kind == TarEntryKind.Directory) return @as(u8, 53); // '5'
    if (kind == TarEntryKind.Fifo) return @as(u8, 54); // '6'
    if (kind == TarEntryKind.GnuLongName) return @as(u8, 76); // 'L'
    if (kind == TarEntryKind.GnuLongLink) return @as(u8, 75); // 'K'
    if (kind == TarEntryKind.PaxHeader) return @as(u8, 120); // 'x'
    if (kind == TarEntryKind.PaxGlobalHeader) return @as(u8, 103); // 'g'
    return @as(u8, 48);
}

/// Encodes a single USTAR TAR entry (512B header + data + 512B zero padding) into `out_buf`.
/// Automatically splits paths > 100 bytes across USTAR `prefix` (155B) + `name` (100B).
pub fn encodeTarEntry(
    path: []const u8,
    kind: TarEntryKind,
    mode: u32,
    mtime: u64,
    link_target: []const u8,
    data: []const u8,
    out_buf: []u8,
    out_pos: *usize,
)! {
    const pad = padTo512(@as(u64, data.len));
    const total_needed = TAR_BLOCK_SIZE + data.len + pad;
    if (out_pos.* + total_needed > out_buf.len) {
        return TarStreamError.BufferTooSmall;
    }

    let hdr: [512]u8 = [0; 512];

    // Split path into prefix + name if > 100 bytes
    if (path.len <= 100) {
        copyBytes(hdr[0..100], path)?;
    } else {
        // Find last '/' such that suffix <= 100 and prefix <= 155
        let split_opt: usize? = null;
        let i: usize = 0;
        while (i < path.len) {
            if (path[i] == '/') {
                const pref_len = i;
                const name_len = path.len - i - 1;
                if (pref_len <= 155 && name_len > 0 && name_len <= 100) {
                    split_opt = i;
                }
            }
            i += 1;
        }
        if (sp := split_opt) {
            copyBytes(hdr[345..500], path[0..sp])?;
            copyBytes(hdr[0..100], path[sp + 1 ..])?;
        } else {
            return TarStreamError.NameTooLongForUstar;
        }
    }

    writeOctalField(hdr[100..108], @as(u64, mode));
    writeOctalField(hdr[108..116], 0);
    writeOctalField(hdr[116..124], 0);
    writeOctalField(hdr[124..136], @as(u64, data.len));
    writeOctalField(hdr[136..148], mtime);

    // Fill checksum field with spaces before summing
    let ci: usize = 148;
    while (ci < 156) {
        hdr[ci] = @as(u8, 32);
        ci += 1;
    }

    hdr[156] = kindToTypeflag(kind);
    if (link_target.len > 0) {
        if (link_target.len > 100) return TarStreamError.NameTooLongForUstar;
        copyBytes(hdr[157..257], link_target)?;
    }

    // USTAR magic + version
    copyBytes(hdr[257..263], "ustar\x00")?;
    hdr[263] = @as(u8, 48);
    hdr[264] = @as(u8, 48);

    // Compute unsigned checksum
    let sum: u64 = 0;
    let si: usize = 0;
    while (si < TAR_BLOCK_SIZE) {
        sum += @as(u64, hdr[si]);
        si += 1;
    }
    writeOctalField(hdr[148..155], sum);
    hdr[155] = @as(u8, 32);

    // Emit header, data, and padding
    let k: usize = 0;
    while (k < TAR_BLOCK_SIZE) {
        out_buf[out_pos.* + k] = hdr[k];
        k += 1;
    }
    out_pos.* += TAR_BLOCK_SIZE;

    k = 0;
    while (k < data.len) {
        out_buf[out_pos.* + k] = data[k];
        k += 1;
    }
    out_pos.* += data.len;

    k = 0;
    while (k < pad) {
        out_buf[out_pos.* + k] = 0;
        k += 1;
    }
    out_pos.* += pad;
}

fn decimalDigitsLen(v: usize): usize {
    if (v == 0) return 1;
    let n = v;
    let d: usize = 0;
    while (n > 0) {
        n = n / 10;
        d += 1;
    }
    return d;
}

/// Encodes a POSIX PAX `x` extended header entry for a single `key=value` record.
pub fn encodeTarPaxHeader(
    pax_key: []const u8,
    pax_val: []const u8,
    out_buf: []u8,
    out_pos: *usize,
)! {
    // Record format: `<total_len> <key>=<val>\n`
    const body_len: usize = pax_key.len + pax_val.len + @as(usize, 3);
    let total_len: usize = body_len + @as(usize, 1);
    while (decimalDigitsLen(total_len) + body_len != total_len) {
        total_len = decimalDigitsLen(total_len) + body_len;
    }

    let rec_buf: [2048]u8 = undefined;
    if (total_len > rec_buf.len) return TarStreamError.BufferTooSmall;

    // Write decimal length
    const dlen = decimalDigitsLen(total_len);
    let n = total_len;
    let di = dlen;
    while (di > 0) {
        di -= 1;
        rec_buf[di] = @intCast(48 + (n % 10));
        n = n / 10;
    }
    let pos = dlen;
    rec_buf[pos] = @as(u8, 32); // ' '
    pos += 1;
    copyBytes(rec_buf[pos..], pax_key)?;
    pos += pax_key.len;
    rec_buf[pos] = @as(u8, 61); // '='
    pos += 1;
    copyBytes(rec_buf[pos..], pax_val)?;
    pos += pax_val.len;
    rec_buf[pos] = @as(u8, 10); // '\n'
    pos += 1;

    encodeTarEntry(
        "PaxHeaders/entry",
        TarEntryKind.PaxHeader,
        0o644,
        0,
        "",
        rec_buf[0..pos],
        out_buf,
        out_pos,
    )?;
}

/// Appends the standard 1024-byte (2 x 512B zero blocks) TAR end-of-archive trailer.
pub fn finalizeTarArchive(out_buf: []u8, out_pos: *usize)! {
    const trailer_len: usize = TAR_BLOCK_SIZE * 2;
    if (out_pos.* + trailer_len > out_buf.len) {
        return TarStreamError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < trailer_len) {
        out_buf[out_pos.* + i] = 0;
        i += 1;
    }
    out_pos.* += trailer_len;
}
