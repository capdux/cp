// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:27:23Z (Linux /proc/<pid>/statm Parser)
// Pure-CP zero-allocation Linux /proc/<pid>/statm 7-column memory page counter parser.

const U64_MAX: u64 = 18446744073709551615;
const U64_MAX_DIV_10: u64 = 1844674407370955161;

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn satMulU64(a: u64, b: u64): u64 {
    if (a == 0 || b == 0) return 0;
    if (a > U64_MAX / b) return U64_MAX;
    return a * b;
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

fn nextToken(content: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < content.len && isWs(content[pos.*])) {
        pos.* += 1;
    }
    if (pos.* >= content.len) return null;
    const start = pos.*;
    while (pos.* < content.len && !isWs(content[pos.*])) {
        pos.* += 1;
    }
    return content[start..pos.*];
}

/// Parsed 7-column memory page counters from Linux `/proc/<pid>/statm`.
pub struct ProcStatm {
    size_pages: u64,
    resident_pages: u64,
    shared_pages: u64,
    text_pages: u64,
    lib_pages: u64,
    data_pages: u64,
    dirty_pages: u64,

    /// Total virtual memory size in bytes (saturating multiplication).
    fn vmBytes(page_size: u64): u64 {
        return satMulU64(_.size_pages, page_size);
    }

    /// Resident set size (RSS) in bytes (saturating multiplication).
    fn rssBytes(page_size: u64): u64 {
        return satMulU64(_.resident_pages, page_size);
    }

    /// Shared resident memory size in bytes (saturating multiplication).
    fn sharedBytes(page_size: u64): u64 {
        return satMulU64(_.shared_pages, page_size);
    }

    /// Text (code) segment size in bytes (saturating multiplication).
    fn textBytes(page_size: u64): u64 {
        return satMulU64(_.text_pages, page_size);
    }

    /// Data + stack size in bytes (saturating multiplication).
    fn dataBytes(page_size: u64): u64 {
        return satMulU64(_.data_pages, page_size);
    }

    /// Private (non-shared) RSS pages using saturating subtraction (`resident_pages - shared_pages`).
    fn privateRssPages(): u64 {
        if (_.resident_pages > _.shared_pages) {
            return _.resident_pages - _.shared_pages;
        }
        return 0;
    }

    /// Private (non-shared) RSS in bytes (saturating multiplication).
    fn privateRssBytes(page_size: u64): u64 {
        return satMulU64(_.privateRssPages(), page_size);
    }
}

/// Parses a 7-column whitespace-separated `/proc/<pid>/statm` payload.
/// Rejects fewer than 7 fields, trailing extra fields, negative/non-digit tokens, and u64 overflows.
pub fn parseProcStatm(content: []const u8): ProcStatm? {
    let pos: usize = 0;

    const t0 = nextToken(content, &pos) ?? return null;
    const t1 = nextToken(content, &pos) ?? return null;
    const t2 = nextToken(content, &pos) ?? return null;
    const t3 = nextToken(content, &pos) ?? return null;
    const t4 = nextToken(content, &pos) ?? return null;
    const t5 = nextToken(content, &pos) ?? return null;
    const t6 = nextToken(content, &pos) ?? return null;

    // Must not have any 8th token
    if (nextToken(content, &pos) != null) return null;

    const size_pages = parseU64Strict(t0) ?? return null;
    const resident_pages = parseU64Strict(t1) ?? return null;
    const shared_pages = parseU64Strict(t2) ?? return null;
    const text_pages = parseU64Strict(t3) ?? return null;
    const lib_pages = parseU64Strict(t4) ?? return null;
    const data_pages = parseU64Strict(t5) ?? return null;
    const dirty_pages = parseU64Strict(t6) ?? return null;

    return ProcStatm {
        size_pages: size_pages,
        resident_pages: resident_pages,
        shared_pages: shared_pages,
        text_pages: text_pages,
        lib_pages: lib_pages,
        data_pages: data_pages,
        dirty_pages: dirty_pages,
    };
}
