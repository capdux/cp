// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:29:10Z (Linux /proc/<pid>/io Parser)
// Pure-CP zero-allocation Linux /proc/<pid>/io process I/O statistics parser.

const U64_MAX: u64 = 18446744073709551615;
const U64_MAX_DIV_10: u64 = 1844674407370955161;

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > U64_MAX - b) return U64_MAX;
    return a + b;
}

fn satSubU64(a: u64, b: u64): u64 {
    if (a > b) return a - b;
    return 0;
}

fn parseU64Strict(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = @as(u64, c - '0');
        if (val > U64_MAX_DIV_10) return null;
        if (val == U64_MAX_DIV_10 && d > 5) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

/// Parsed process I/O statistics from Linux `/proc/<pid>/io`.
pub struct ProcIoStats {
    rchar: u64,
    wchar: u64,
    syscr: u64,
    syscw: u64,
    read_bytes: u64,
    write_bytes: u64,
    cancelled_write_bytes: u64,

    /// Net storage bytes written (`write_bytes - cancelled_write_bytes`, saturating).
    fn netWriteBytes(): u64 {
        return satSubU64(_.write_bytes, _.cancelled_write_bytes);
    }

    /// Total storage I/O bytes (`read_bytes + netWriteBytes()`, saturating).
    fn totalStorageBytes(): u64 {
        return satAddU64(_.read_bytes, _.netWriteBytes());
    }

    /// Total read + write I/O syscalls (`syscr + syscw`, saturating).
    fn totalSyscalls(): u64 {
        return satAddU64(_.syscr, _.syscw);
    }

    /// Total character-level bytes read + written (`rchar + wchar`, saturating).
    fn totalCharBytes(): u64 {
        return satAddU64(_.rchar, _.wchar);
    }

    /// Per-field saturating delta (`self - earlier`) for sampling-window throughput calculation.
    fn diff(earlier: ProcIoStats): ProcIoStats {
        return ProcIoStats {
            rchar: satSubU64(_.rchar, earlier.rchar),
            wchar: satSubU64(_.wchar, earlier.wchar),
            syscr: satSubU64(_.syscr, earlier.syscr),
            syscw: satSubU64(_.syscw, earlier.syscw),
            read_bytes: satSubU64(_.read_bytes, earlier.read_bytes),
            write_bytes: satSubU64(_.write_bytes, earlier.write_bytes),
            cancelled_write_bytes: satSubU64(_.cancelled_write_bytes, earlier.cancelled_write_bytes),
        };
    }
}

/// Single-pass zero-allocation parser for Linux `/proc/<pid>/io` `key: value` lines.
/// Supports `\n` and `\r\n`, skips unknown extension keys, rejects invalid/overflowing values on known keys,
/// and returns `null` if no known key is found.
pub fn parseProcIo(content: []const u8): ProcIoStats? {
    let stats = ProcIoStats {
        rchar: 0,
        wchar: 0,
        syscr: 0,
        syscw: 0,
        read_bytes: 0,
        write_bytes: 0,
        cancelled_write_bytes: 0,
    };
    let found_any: bool = false;

    let pos: usize = 0;
    while (pos < content.len) {
        const line_start = pos;
        while (pos < content.len && content[pos] != '\n') {
            pos += 1;
        }
        let line_end = pos;
        if (pos < content.len && content[pos] == '\n') {
            pos += 1;
        }
        if (line_end > line_start && content[line_end - 1] == '\r') {
            line_end -= 1;
        }

        const line = trimWs(content[line_start..line_end]);
        if (line.len == 0) continue;

        let colon_idx: usize = 0;
        while (colon_idx < line.len && line[colon_idx] != ':') {
            colon_idx += 1;
        }
        if (colon_idx == 0 || colon_idx >= line.len) continue;

        const key = trimWs(line[0..colon_idx]);
        const val_str = trimWs(line[colon_idx + 1..line.len]);

        if (strEq(key, "rchar")) {
            stats.rchar = parseU64Strict(val_str) ?? return null;
            found_any = true;
        } else if (strEq(key, "wchar")) {
            stats.wchar = parseU64Strict(val_str) ?? return null;
            found_any = true;
        } else if (strEq(key, "syscr")) {
            stats.syscr = parseU64Strict(val_str) ?? return null;
            found_any = true;
        } else if (strEq(key, "syscw")) {
            stats.syscw = parseU64Strict(val_str) ?? return null;
            found_any = true;
        } else if (strEq(key, "read_bytes")) {
            stats.read_bytes = parseU64Strict(val_str) ?? return null;
            found_any = true;
        } else if (strEq(key, "write_bytes")) {
            stats.write_bytes = parseU64Strict(val_str) ?? return null;
            found_any = true;
        } else if (strEq(key, "cancelled_write_bytes")) {
            stats.cancelled_write_bytes = parseU64Strict(val_str) ?? return null;
            found_any = true;
        }
    }

    if (!found_any) return null;
    return stats;
}
