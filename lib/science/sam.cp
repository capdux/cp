// Origin: Pure-CP Stdlib (Zero-Alloc SAM v1.6 Sequence Alignment/Map Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation SAM v1.6 parser: 12 bitwise FLAG masks, CIGAR iterator
// with reference/query span calculators, @HD/@SQ/@RG header parser, 11-column alignment
// record & TAG:TYPE:VALUE lookup, and single-pass alignment statistics summarizer.

const std = @import("std");

// Standard SAM v1.6 Bitwise FLAG Masks
pub const SAM_FLAG_PAIRED: u16 = 0x0001;
pub const SAM_FLAG_PROPER_PAIR: u16 = 0x0002;
pub const SAM_FLAG_UNMAPPED: u16 = 0x0004;
pub const SAM_FLAG_MATE_UNMAPPED: u16 = 0x0008;
pub const SAM_FLAG_REVERSE: u16 = 0x0010;
pub const SAM_FLAG_MATE_REVERSE: u16 = 0x0020;
pub const SAM_FLAG_READ1: u16 = 0x0040;
pub const SAM_FLAG_READ2: u16 = 0x0080;
pub const SAM_FLAG_SECONDARY: u16 = 0x0100;
pub const SAM_FLAG_QC_FAIL: u16 = 0x0200;
pub const SAM_FLAG_DUPLICATE: u16 = 0x0400;
pub const SAM_FLAG_SUPPLEMENTARY: u16 = 0x0800;

/// Single decoded CIGAR operation (`len` > 0 and `op` in M/I/D/N/S/H/P/=/X).
pub struct CigarOp {
    len: u32 = 0,
    op: u8 = 0,
}

/// Zero-copy parsed SAM header line (`@HD`, `@SQ`, `@RG`, `@PG`, `@CO`).
pub struct SamHeaderLine {
    record_type: []const u8 = "",
    fields_slice: []const u8 = "",
}

/// Zero-copy parsed SAM alignment record (11 mandatory columns + optional tags slice).
pub struct SamRecord {
    qname: []const u8 = "",
    flag: u16 = 0,
    rname: []const u8 = "",
    pos: u64 = 0,
    mapq: u8 = 0,
    cigar: []const u8 = "",
    rnext: []const u8 = "",
    pnext: u64 = 0,
    tlen: i64 = 0,
    seq: []const u8 = "",
    qual: []const u8 = "",
    optional_tags: []const u8 = "",

    fn isMapped(): bool {
        return (_.flag & SAM_FLAG_UNMAPPED) == 0;
    }

    fn isReverse(): bool {
        return (_.flag & SAM_FLAG_REVERSE) != 0;
    }

    fn isPaired(): bool {
        return (_.flag & SAM_FLAG_PAIRED) != 0;
    }

    fn isProperPair(): bool {
        return (_.flag & SAM_FLAG_PROPER_PAIR) != 0;
    }

    fn isSecondary(): bool {
        return (_.flag & SAM_FLAG_SECONDARY) != 0;
    }

    fn isDuplicate(): bool {
        return (_.flag & SAM_FLAG_DUPLICATE) != 0;
    }

    fn isSupplementary(): bool {
        return (_.flag & SAM_FLAG_SUPPLEMENTARY) != 0;
    }

    /// Looks up a 2-byte optional tag (`TAG:TYPE:VALUE`) in `optional_tags` and returns `VALUE`.
    fn findTag(tag_2b: []const u8): ([]const u8)? {
        if (tag_2b.len != 2 || _.optional_tags.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.optional_tags.len) {
            const start = i;
            while (i < _.optional_tags.len && _.optional_tags[i] != '\t') {
                i += 1;
            }
            const field = _.optional_tags[start..i];
            if (i < _.optional_tags.len) {
                i += 1;
            }
            if (field.len >= 5 &&
                field[0] == tag_2b[0] &&
                field[1] == tag_2b[1] &&
                field[2] == ':' &&
                field[4] == ':')
            {
                return field[5..];
            }
        }
        return null;
    }
}

/// Aggregated single-pass SAM alignment statistics.
pub struct SamStats {
    total_reads: u64 = 0,
    mapped_reads: u64 = 0,
    unmapped_reads: u64 = 0,
    paired_reads: u64 = 0,
    proper_pair_reads: u64 = 0,
    reverse_reads: u64 = 0,
    duplicate_reads: u64 = 0,
    secondary_reads: u64 = 0,
}

struct TabField {
    value: []const u8,
    next_pos: usize,
    has_tab: bool,
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn trimLineEnd(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\n' || line[end - 1] == '\r')) {
        end -= 1;
    }
    return line[0..end];
}

fn nextTabField(line: []const u8, pos: usize): TabField? {
    if (pos > line.len) {
        return null;
    }
    let i = pos;
    while (i < line.len && line[i] != '\t') {
        i += 1;
    }
    const has_tab = i < line.len;
    return TabField {
        value: line[pos..i],
        next_pos: if (has_tab) (i + 1) else line.len,
        has_tab: has_tab,
    };
}

fn isValidCigarOp(b: u8): bool {
    return b == 'M' || b == 'I' || b == 'D' || b == 'N' ||
        b == 'S' || b == 'H' || b == 'P' || b == '=' || b == 'X';
}

fn consumesRef(op: u8): bool {
    return op == 'M' || op == 'D' || op == 'N' || op == '=' || op == 'X';
}

fn consumesQuery(op: u8): bool {
    return op == 'M' || op == 'I' || op == 'S' || op == '=' || op == 'X';
}

fn parseU8Dec(s: []const u8): u8? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(u8, s, 10) catch return null;
}

fn parseU16Dec(s: []const u8): u16? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(u16, s, 10) catch return null;
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(u64, s, 10) catch return null;
}

fn parseI64Dec(s: []const u8): i64? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(i64, s, 10) catch return null;
}

// ---------------------------------------------------------------------------
// 1. CIGAR Zero-Alloc Iterator & Span Calculators
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over CIGAR operations (e.g. `"50M2I10D5S"`).
/// Yields nothing for `"*"`; sets `had_error = true` on zero-length, overflow, or invalid op.
pub struct CigarIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,
    had_error: bool = false,

    static fn init(cigar: []const u8): CigarIter {
        if (cigar.len == 0) {
            return CigarIter { src: cigar, pos: 0, done: true, had_error: true };
        }
        if (cigar.len == 1 && cigar[0] == '*') {
            return CigarIter { src: cigar, pos: 1, done: true, had_error: false };
        }
        return CigarIter { src: cigar, pos: 0, done: false, had_error: false };
    }

    fn next(*_): CigarOp? {
        if (_.done) {
            return null;
        }
        if (_.pos >= _.src.len) {
            _.done = true;
            return null;
        }

        let acc: u64 = 0;
        let digits: usize = 0;
        while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
            acc = acc * 10 + ((_.src[_.pos] - '0') as u64);
            if (acc > 4294967295) {
                _.done = true;
                _.had_error = true;
                return null;
            }
            digits += 1;
            _.pos += 1;
        }

        if (digits == 0 || acc == 0 || _.pos >= _.src.len) {
            _.done = true;
            _.had_error = true;
            return null;
        }

        const op = _.src[_.pos];
        if (!isValidCigarOp(op)) {
            _.done = true;
            _.had_error = true;
            return null;
        }
        _.pos += 1;
        if (_.pos >= _.src.len) {
            _.done = true;
        }

        return CigarOp {
            len: acc as u32,
            op: op,
        };
    }
}

/// Computes total reference-consuming length (`M`, `D`, `N`, `=`, `X`) of a CIGAR string.
/// Returns `0` for `"*"` and `null` for malformed CIGAR strings.
pub fn cigarRefLen(cigar: []const u8): u64? {
    let it = CigarIter.init(cigar);
    let total: u64 = 0;
    while (op := it.next()) {
        if (consumesRef(op.op)) {
            total += op.len as u64;
        }
    }
    if (it.had_error) {
        return null;
    }
    return total;
}

/// Computes total query-consuming length (`M`, `I`, `S`, `=`, `X`) of a CIGAR string.
/// Returns `0` for `"*"` and `null` for malformed CIGAR strings.
pub fn cigarQueryLen(cigar: []const u8): u64? {
    let it = CigarIter.init(cigar);
    let total: u64 = 0;
    while (op := it.next()) {
        if (consumesQuery(op.op)) {
            total += op.len as u64;
        }
    }
    if (it.had_error) {
        return null;
    }
    return total;
}

// ---------------------------------------------------------------------------
// 2. SAM Header Line Iterator & Tag Lookup
// ---------------------------------------------------------------------------

/// Looks up a 2-byte header tag (e.g. `"VN"`, `"SN"`, `"LN"`, `"SO"`) in `fields_slice`.
pub fn findSamHeaderTag(fields_slice: []const u8, tag_2b: []const u8): ([]const u8)? {
    if (tag_2b.len != 2 || fields_slice.len == 0) {
        return null;
    }
    let i: usize = 0;
    while (i < fields_slice.len) {
        const start = i;
        while (i < fields_slice.len && fields_slice[i] != '\t') {
            i += 1;
        }
        const field = fields_slice[start..i];
        if (i < fields_slice.len) {
            i += 1;
        }
        if (field.len >= 3 &&
            field[0] == tag_2b[0] &&
            field[1] == tag_2b[1] &&
            field[2] == ':')
        {
            return field[3..];
        }
    }
    return null;
}

/// Zero-allocation iterator over `@`-prefixed SAM header lines (`@HD`, `@SQ`, `@RG`, `@PG`, `@CO`).
pub struct SamHeaderIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    fn init(data: []const u8): SamHeaderIter {
        return SamHeaderIter {
            src: data,
            pos: 0,
            done: data.len == 0,
        };
    }

    fn next(*_): SamHeaderLine? {
        if (_.done) {
            return null;
        }
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            let i = line_start;
            while (i < _.src.len && _.src[i] != '\n') {
                i += 1;
            }
            const line = trimLineEnd(_.src[line_start..i]);
            _.pos = if (i < _.src.len) (i + 1) else _.src.len;

            if (line.len == 0) {
                continue;
            }
            if (line[0] != '@') {
                _.done = true;
                return null;
            }
            if (line.len < 3) {
                continue;
            }
            const rtype = line[1..3];
            let fields: []const u8 = "";
            if (line.len > 3 && line[3] == '\t') {
                fields = line[4..];
            }
            return SamHeaderLine {
                record_type: rtype,
                fields_slice: fields,
            };
        }
        _.done = true;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 3. SAM Alignment Record Parser, Iterator & Single-Pass Summarizer
// ---------------------------------------------------------------------------

/// Parses a single SAM v1.6 alignment line (11 mandatory tab-separated columns + optional tags).
/// Validates FLAG, POS, MAPQ, CIGAR syntax, PNEXT, and TLEN.
pub fn parseSamLine(line: []const u8): SamRecord? {
    const trimmed = trimLineEnd(line);
    if (trimmed.len == 0 || trimmed[0] == '@') {
        return null;
    }

    const f0 = nextTabField(trimmed, 0) ?? return null;
    if (!f0.has_tab || f0.value.len == 0) return null;

    const f1 = nextTabField(trimmed, f0.next_pos) ?? return null;
    if (!f1.has_tab) return null;

    const f2 = nextTabField(trimmed, f1.next_pos) ?? return null;
    if (!f2.has_tab || f2.value.len == 0) return null;

    const f3 = nextTabField(trimmed, f2.next_pos) ?? return null;
    if (!f3.has_tab) return null;

    const f4 = nextTabField(trimmed, f3.next_pos) ?? return null;
    if (!f4.has_tab) return null;

    const f5 = nextTabField(trimmed, f4.next_pos) ?? return null;
    if (!f5.has_tab || f5.value.len == 0) return null;

    const f6 = nextTabField(trimmed, f5.next_pos) ?? return null;
    if (!f6.has_tab || f6.value.len == 0) return null;

    const f7 = nextTabField(trimmed, f6.next_pos) ?? return null;
    if (!f7.has_tab) return null;

    const f8 = nextTabField(trimmed, f7.next_pos) ?? return null;
    if (!f8.has_tab) return null;

    const f9 = nextTabField(trimmed, f8.next_pos) ?? return null;
    if (!f9.has_tab || f9.value.len == 0) return null;

    const f10 = nextTabField(trimmed, f9.next_pos) ?? return null;
    if (f10.value.len == 0) return null;

    const flag = parseU16Dec(f1.value) ?? return null;
    const pos = parseU64Dec(f3.value) ?? return null;
    const mapq = parseU8Dec(f4.value) ?? return null;
    // Validate CIGAR syntax (must be "*" or valid op sequence)
    * = cigarRefLen(f5.value) ?? return null;
    const pnext = parseU64Dec(f7.value) ?? return null;
    const tlen = parseI64Dec(f8.value) ?? return null;

    const opt_tags = if (f10.has_tab) trimmed[f10.next_pos..] else "";

    return SamRecord {
        qname: f0.value,
        flag: flag,
        rname: f2.value,
        pos: pos,
        mapq: mapq,
        cigar: f5.value,
        rnext: f6.value,
        pnext: pnext,
        tlen: tlen,
        seq: f9.value,
        qual: f10.value,
        optional_tags: opt_tags,
    };
}

/// Zero-allocation iterator over SAM alignment records (skips `@` header lines and blank lines).
pub struct SamRecordIter {
    src: []const u8 = "",
    pos: usize = 0,
    had_error: bool = false,

    static fn init(data: []const u8): SamRecordIter {
        return SamRecordIter {
            src: data,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): SamRecord? {
        if (_.had_error) {
            return null;
        }
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            let i = line_start;
            while (i < _.src.len && _.src[i] != '\n') {
                i += 1;
            }
            const line = trimLineEnd(_.src[line_start..i]);
            _.pos = if (i < _.src.len) (i + 1) else _.src.len;

            if (line.len == 0 || line[0] == '@') {
                continue;
            }
            if (rec := parseSamLine(line)) {
                return rec;
            }
            _.had_error = true;
            return null;
        }
        return null;
    }
}

/// Single-pass SAM alignment statistics summarizer. Returns `null` if any alignment record is malformed.
pub fn summarizeSam(data: []const u8): SamStats? {
    let stats = SamStats {};
    let it = SamRecordIter.init(data);

    while (rec := it.next()) {
        stats.total_reads += 1;
        if (rec.isMapped()) {
            stats.mapped_reads += 1;
        } else {
            stats.unmapped_reads += 1;
        }
        if (rec.isPaired()) {
            stats.paired_reads += 1;
        }
        if (rec.isProperPair()) {
            stats.proper_pair_reads += 1;
        }
        if (rec.isReverse()) {
            stats.reverse_reads += 1;
        }
        if (rec.isDuplicate()) {
            stats.duplicate_reads += 1;
        }
        if (rec.isSecondary()) {
            stats.secondary_reads += 1;
        }
    }

    if (it.had_error) {
        return null;
    }
    return stats;
}
