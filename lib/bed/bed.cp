// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc UCSC BED3-BED12 Genomic Interval Parser)

/// UCSC BED 链方向枚举
pub enum BedStrand: u8 {
    plus = '+',
    minus = '-',
    unknown = '.',
}

/// UCSC BED3–BED12 基因组区间零拷贝记录（0-based 半开区间 `[start, end)`）
pub struct BedRecord {
    chrom: []const u8,
    start: u64,
    end: u64,
    name: []const u8 = "",
    score: u32 = 0,
    strand: BedStrand = BedStrand.unknown,
    thick_start: u64 = 0,
    thick_end: u64 = 0,
    item_rgb: []const u8 = "",
    block_count: u32 = 0,
    block_sizes: []const u8 = "",
    block_starts: []const u8 = "",
    field_count: u8 = 3,

    /// 区间碱基长度 (`end - start`)
    pub fn length(): u64 {
        return _.end - _.start;
    }

    /// 判定是否与另一 BED 区间重叠（同染色体且 `start < other.end && other.start < end`）
    pub fn overlaps(other: BedRecord): bool {
        if (!strEql(_.chrom, other.chrom)) return false;
        return _.start < other.end && other.start < _.end;
    }

    /// 计算与另一 BED 区间的重叠碱基数（跨染色体或不重叠返回 `0`）
    pub fn overlapLen(other: BedRecord): u64 {
        if (!_.overlaps(other)) return 0;
        const lo = if (_.start > other.start) _.start else other.start;
        const hi = if (_.end < other.end) _.end else other.end;
        return hi - lo;
    }

    /// 判定 0-based 坐标 `pos` 是否落入半开区间 `[start, end)`
    pub fn containsPos(pos: u64): bool {
        return pos >= _.start && pos < _.end;
    }
}

/// BED 数据集统计汇总
pub struct BedStats {
    record_count: usize = 0,
    total_span_bp: u64 = 0,
    min_len: u64 = 0,
    max_len: u64 = 0,
    plus_count: usize = 0,
    minus_count: usize = 0,
    min_fields: u8 = 0,
    max_fields: u8 = 0,
}

fn isHSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWithWord(line: []const u8, word: []const u8): bool {
    if (line.len < word.len) return false;
    let i: usize = 0;
    while (i < word.len) {
        if (line[i] != word[i]) return false;
        i += 1;
    }
    if (line.len == word.len) return true;
    return isHSpace(line[word.len]);
}

fn trimLine(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (isHSpace(s[end - 1]) || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = u64(c - '0');
        if (val > (18446744073709551615 - d) / 10) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

fn parseU32Dec(s: []const u8): u32? {
    const v = parseU64Dec(s) ?? return null;
    if (v > 4294967295) return null;
    return v as u32;
}

/// 解析单行 BED3–BED12 记录（支持 Tab 及空白分隔，自动跳过末尾 `\r`；校验 `start <= end` 与列类型）
pub fn parseBedLine(line: []const u8): BedRecord? {
    const s = trimLine(line);
    if (s.len == 0) return null;

    let cols: [12][]const u8 = undefined;
    let ncols: usize = 0;
    let i: usize = 0;

    while (i < s.len && ncols < 12) {
        while (i < s.len && isHSpace(s[i])) {
            i += 1;
        }
        if (i >= s.len) break;
        const start = i;
        while (i < s.len && !isHSpace(s[i])) {
            i += 1;
        }
        cols[ncols] = s[start..i];
        ncols += 1;
    }

    if (ncols < 3) return null;

    const chrom = cols[0];
    const start_pos = parseU64Dec(cols[1]) ?? return null;
    const end_pos = parseU64Dec(cols[2]) ?? return null;
    if (start_pos > end_pos) return null;

    let rec = BedRecord {
        chrom: chrom,
        start: start_pos,
        end: end_pos,
        field_count: ncols as u8,
    };

    if (ncols >= 4) {
        rec.name = cols[3];
    }
    if (ncols >= 5) {
        if (!strEql(cols[4], ".")) {
            rec.score = parseU32Dec(cols[4]) ?? return null;
        }
    }
    if (ncols >= 6) {
        if (cols[5].len != 1) return null;
        const sc = cols[5][0];
        if (sc == '+') {
            rec.strand = BedStrand.plus;
        } else if (sc == '-') {
            rec.strand = BedStrand.minus;
        } else if (sc == '.') {
            rec.strand = BedStrand.unknown;
        } else {
            return null;
        }
    }
    if (ncols >= 7) {
        rec.thick_start = parseU64Dec(cols[6]) ?? return null;
    }
    if (ncols >= 8) {
        rec.thick_end = parseU64Dec(cols[7]) ?? return null;
        if (rec.thick_start > rec.thick_end) return null;
    }
    if (ncols >= 9) {
        rec.item_rgb = cols[8];
    }
    if (ncols >= 10) {
        rec.block_count = parseU32Dec(cols[9]) ?? return null;
    }
    if (ncols >= 11) {
        rec.block_sizes = cols[10];
    }
    if (ncols >= 12) {
        rec.block_starts = cols[11];
    }

    return rec;
}

/// 零堆分配 BED 文件流式迭代器（自动跳过空行、`#` 注释、`track ...` 与 `browser ...` UCSC 头行）
pub struct BedIter {
    data: []const u8,
    pos: usize = 0,
    had_error: bool = false,

    pub static fn init(data: []const u8): BedIter {
        let start: usize = 0;
        if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
            start = 3;
        }
        return BedIter {
            data: data,
            pos: start,
            had_error: false,
        };
    }

    pub fn next(*_): BedRecord? {
        if (_.had_error) return null;

        while (_.pos < _.data.len) {
            const line_start = _.pos;
            while (_.pos < _.data.len && _.data[_.pos] != '\n') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.data.len && _.data[_.pos] == '\n') {
                _.pos += 1;
            }

            const trimmed = trimLine(_.data[line_start..line_end]);
            if (trimmed.len == 0) continue;
            if (trimmed[0] == '#') continue;
            if (startsWithWord(trimmed, "track") || startsWithWord(trimmed, "browser")) {
                continue;
            }

            const rec_opt = parseBedLine(trimmed);
            if (rec_opt == null) {
                _.had_error = true;
                return null;
            }
            return rec_opt;
        }

        return null;
    }
}

/// 单遍统计 BED 文件的记录数、总覆盖碱基跨度、长度极值、正负链计数与列数范围
pub fn summarizeBed(data: []const u8): BedStats {
    let stats = BedStats {};
    let it = BedIter.init(data);

    while (rec := it.next()) {
        const rlen = rec.length();
        if (stats.record_count == 0) {
            stats.min_len = rlen;
            stats.max_len = rlen;
            stats.min_fields = rec.field_count;
            stats.max_fields = rec.field_count;
        } else {
            if (rlen < stats.min_len) stats.min_len = rlen;
            if (rlen > stats.max_len) stats.max_len = rlen;
            if (rec.field_count < stats.min_fields) stats.min_fields = rec.field_count;
            if (rec.field_count > stats.max_fields) stats.max_fields = rec.field_count;
        }

        stats.record_count += 1;
        stats.total_span_bp += rlen;
        if (rec.strand == BedStrand.plus) {
            stats.plus_count += 1;
        } else if (rec.strand == BedStrand.minus) {
            stats.minus_count += 1;
        }
    }

    return stats;
}
