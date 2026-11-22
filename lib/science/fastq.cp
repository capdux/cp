// Origin: Pure-CP Stdlib (Zero-Alloc Sanger/Illumina FASTQ Sequencing Parser & QC) | Created: 2026-11-22

/// Sanger / Illumina 1.8+ Phred+33 质量偏移量
pub const PHRED_OFFSET_33: u8 = 33;

/// 旧版 Illumina 1.3–1.7 Phred+64 质量偏移量
pub const PHRED_OFFSET_64: u8 = 64;

/// 单条 FASTQ 测序读段（零拷贝切片指向原始输入缓冲区）
pub struct FastqRecord {
    id: []const u8,
    description: []const u8,
    seq: []const u8,
    qual: []const u8,

    /// 读取索引 `idx` 处的 Phred 质量分（防下溢与越界）
    pub fn phredAt(idx: usize, offset: u8): u8? {
        if (idx >= _.qual.len) {
            return null;
        }
        const q = _.qual[idx];
        if (q < offset) {
            return null;
        }
        return q - offset;
    }

    /// 计算读段平均 Phred 质量分
    pub fn meanQuality(offset: u8): f64 {
        if (_.qual.len == 0) {
            return 0.0;
        }
        let sum: u64 = 0;
        let i: usize = 0;
        while (i < _.qual.len) {
            const q = _.qual[i];
            if (q >= offset) {
                sum += u64(q - offset);
            }
            i += 1;
        }
        return f64(sum) / f64(_.qual.len);
    }

    /// 计算 GC 含量 `(G + C) / (A + T + U + G + C)`（忽略大小写）
    pub fn gcContent(): f64 {
        let gc: u64 = 0;
        let total: u64 = 0;
        let i: usize = 0;
        while (i < _.seq.len) {
            const b = _.seq[i];
            if (b == 'G' || b == 'g' || b == 'C' || b == 'c') {
                gc += 1;
                total += 1;
            } else if (b == 'A' || b == 'a' || b == 'T' || b == 't' || b == 'U' || b == 'u') {
                total += 1;
            }
            i += 1;
        }
        if (total == 0) {
            return 0.0;
        }
        return f64(gc) / f64(total);
    }

    /// 返回读段中的最低 Phred 质量分
    pub fn minQuality(offset: u8): u8 {
        if (_.qual.len == 0) {
            return 0;
        }
        let min_q: u8 = 255;
        let i: usize = 0;
        while (i < _.qual.len) {
            const q = _.qual[i];
            const phred: u8 = if (q >= offset) (q - offset) else 0;
            if (phred < min_q) {
                min_q = phred;
            }
            i += 1;
        }
        return min_q;
    }

    /// 计算 3' 端连续低于 `min_q` 碱基裁切后的安全前缀长度（零拷贝质量修剪）
    pub fn trimTailEnd(min_q: u8, offset: u8): usize {
        let end = _.qual.len;
        while (end > 0) {
            const q = _.qual[end - 1];
            const phred: u8 = if (q >= offset) (q - offset) else 0;
            if (phred >= min_q) {
                break;
            }
            end -= 1;
        }
        return end;
    }
}

/// 完整 FASTQ 数据的单遍质控统计汇总
pub struct FastqStats {
    read_count: u64,
    total_bases: u64,
    q20_bases: u64,
    q30_bases: u64,
    gc_bases: u64,
    n_bases: u64,
    min_len: usize,
    max_len: usize,

    pub fn q20Rate(): f64 {
        if (_.total_bases == 0) return 0.0;
        return f64(_.q20_bases) / f64(_.total_bases);
    }

    pub fn q30Rate(): f64 {
        if (_.total_bases == 0) return 0.0;
        return f64(_.q30_bases) / f64(_.total_bases);
    }

    pub fn gcRate(): f64 {
        if (_.total_bases == 0) return 0.0;
        return f64(_.gc_bases) / f64(_.total_bases);
    }
}

// ---------------------------------------------------------------------------
// 内部零分配行扫描辅助函数
// ---------------------------------------------------------------------------

fn isLineSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

struct LineView {
    text: []const u8,
    next_pos: usize,
}

fn readLine(data: []const u8, pos: usize): LineView? {
    if (pos >= data.len) {
        return null;
    }
    let end = pos;
    while (end < data.len && data[end] != '\n') {
        end += 1;
    }
    const next_pos = if (end < data.len && data[end] == '\n') (end + 1) else end;
    let text_end = end;
    if (text_end > pos && data[text_end - 1] == '\r') {
        text_end -= 1;
    }
    return LineView {
        text: data[pos..text_end],
        next_pos: next_pos,
    };
}

// ---------------------------------------------------------------------------
// 1. FASTQ 4 行读段零分配迭代器与格式探测
// ---------------------------------------------------------------------------

/// 零分配逐条迭代 4 行 FASTQ 读段（兼容 `\r\n`/`\n`、跳过记录间空行，畸形置 `had_error = true`）
pub struct FastqIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    pub fn init(data: []const u8): FastqIter {
        return FastqIter {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    fn fail(*_): FastqRecord? {
        _.had_error = true;
        _.pos = _.data.len;
        return null;
    }

    fn nextNonEmptyLine(*_): ([]const u8)? {
        while (lv := readLine(_.data, _.pos)) {
            _.pos = lv.next_pos;
            if (lv.text.len > 0) {
                return lv.text;
            }
        }
        return null;
    }

    pub fn next(*_): FastqRecord? {
        const header_line = _.nextNonEmptyLine() ?? return null;

        if (header_line.len < 2 || header_line[0] != '@') {
            return _.fail();
        }

        const seq_lv = readLine(_.data, _.pos) ?? return _.fail();
        _.pos = seq_lv.next_pos;

        const plus_lv = readLine(_.data, _.pos) ?? return _.fail();
        _.pos = plus_lv.next_pos;
        if (plus_lv.text.len == 0 || plus_lv.text[0] != '+') {
            return _.fail();
        }

        const qual_lv = readLine(_.data, _.pos) ?? return _.fail();
        _.pos = qual_lv.next_pos;

        if (seq_lv.text.len != qual_lv.text.len) {
            return _.fail();
        }

        // 切分 `@id description`
        const hdr_body = header_line[1..];
        let id_end: usize = 0;
        while (id_end < hdr_body.len && !isLineSpace(hdr_body[id_end])) {
            id_end += 1;
        }
        const id = hdr_body[0..id_end];
        let desc_start = id_end;
        while (desc_start < hdr_body.len && isLineSpace(hdr_body[desc_start])) {
            desc_start += 1;
        }
        const description = hdr_body[desc_start..];

        return FastqRecord {
            id: id,
            description: description,
            seq: seq_lv.text,
            qual: qual_lv.text,
        };
    }
}

/// 快速校验数据首字节为 `'@'` 且包含完整的 4 行 FASTQ 记录（第 3 行 `'+'` 且序列与质量等长）
pub fn isFastq(data: []const u8): bool {
    if (data.len == 0 || data[0] != '@') {
        return false;
    }
    let it = FastqIter.init(data);
    if (rec := it.next()) {
        * = rec;
        return !it.had_error;
    }
    return false;
}

// ---------------------------------------------------------------------------
// 2. 单遍质控汇总统计 (Q20 / Q30 / GC / N)
// ---------------------------------------------------------------------------

/// 单遍扫描整份 FASTQ 数据，统计读段数、总碱基数、Q20/Q30 碱基数、GC/N 碱基数及长度范围
pub fn summarizeFastq(data: []const u8, offset: u8): FastqStats {
    let stats = FastqStats {
        read_count: 0,
        total_bases: 0,
        q20_bases: 0,
        q30_bases: 0,
        gc_bases: 0,
        n_bases: 0,
        min_len: 0,
        max_len: 0,
    };

    let it = FastqIter.init(data);
    while (rec := it.next()) {
        const n = rec.seq.len;
        if (stats.read_count == 0) {
            stats.min_len = n;
            stats.max_len = n;
        } else {
            if (n < stats.min_len) stats.min_len = n;
            if (n > stats.max_len) stats.max_len = n;
        }
        stats.read_count += 1;
        stats.total_bases += u64(n);

        let i: usize = 0;
        while (i < n) {
            const base = rec.seq[i];
            if (base == 'G' || base == 'g' || base == 'C' || base == 'c') {
                stats.gc_bases += 1;
            } else if (base == 'N' || base == 'n') {
                stats.n_bases += 1;
            }

            const q = rec.qual[i];
            if (q >= offset) {
                const phred = q - offset;
                if (phred >= 20) {
                    stats.q20_bases += 1;
                    if (phred >= 30) {
                        stats.q30_bases += 1;
                    }
                }
            }
            i += 1;
        }
    }

    return stats;
}
