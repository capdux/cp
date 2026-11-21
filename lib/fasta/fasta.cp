// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc NCBI/EMBL FASTA Bioinformatics Parser & IUPAC Reverse Complement)

/// 单条 FASTA 序列零拷贝视图
pub struct FastaRecord {
    id: []const u8,
    description: []const u8,
    raw_seq: []const u8,

    /// 忽略 `\r` / `\n` 与空白统计真实碱基/氨基酸长度
    pub fn seqLen(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.raw_seq.len) {
            if (!isSpace(_.raw_seq[i])) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// 计算 GC 含量 `(G + C) / (A + T + U + G + C)`（大小写不敏感；无有效碱基返回 `0.0`）
    pub fn gcContent(): f64 {
        let gc: usize = 0;
        let at: usize = 0;
        let i: usize = 0;
        while (i < _.raw_seq.len) {
            const c = _.raw_seq[i];
            if (c == 'G' || c == 'g' || c == 'C' || c == 'c') {
                gc += 1;
            } else if (c == 'A' || c == 'a' || c == 'T' || c == 't' || c == 'U' || c == 'u') {
                at += 1;
            }
            i += 1;
        }
        const denom = gc + at;
        if (denom == 0) return 0.0;
        return (gc as f64) / (denom as f64);
    }

    /// 剥离换行与空白，将干净序列拷贝至 `out`（缓冲区不足返回 `null`）
    pub fn copyCleanSeq(out: []u8): usize? {
        let out_len: usize = 0;
        let i: usize = 0;
        while (i < _.raw_seq.len) {
            const c = _.raw_seq[i];
            if (!isSpace(c)) {
                if (out_len >= out.len) return null;
                out[out_len] = c;
                out_len += 1;
            }
            i += 1;
        }
        return out_len;
    }
}

/// FASTA 数据集统计汇总
pub struct FastaStats {
    record_count: usize = 0,
    total_bases: usize = 0,
    min_len: usize = 0,
    max_len: usize = 0,
    gc_bases: usize = 0,
    at_bases: usize = 0,
    n_bases: usize = 0,

    pub fn overallGc(): f64 {
        const denom = _.gc_bases + _.at_bases;
        if (denom == 0) return 0.0;
        return (_.gc_bases as f64) / (denom as f64);
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isHSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimLine(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (isHSpace(s[end - 1]) || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

/// 跳过可选 UTF-8 BOM、前导空行与 `;` 注释行
fn skipLeadingTrivia(data: []const u8): usize {
    let pos: usize = 0;
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        pos = 3;
    }
    while (pos < data.len) {
        const line_start = pos;
        while (pos < data.len && data[pos] != '\n') {
            pos += 1;
        }
        const line_end = pos;
        const next_pos = if (pos < data.len && data[pos] == '\n') pos + 1 else pos;
        const trimmed = trimLine(data[line_start..line_end]);
        if (trimmed.len == 0 || trimmed[0] == ';') {
            pos = next_pos;
            continue;
        }
        return line_start;
    }
    return data.len;
}

/// 跳过前导空行与 `;` 注释行后校验是否以 `'>'` 开头
pub fn isFasta(data: []const u8): bool {
    const pos = skipLeadingTrivia(data);
    if (pos >= data.len) return false;
    let i = pos;
    while (i < data.len && isHSpace(data[i])) {
        i += 1;
    }
    return i < data.len && data[i] == '>';
}

/// 零堆分配 FASTA 多记录流式迭代器（兼容 `\r\n`/`\n`、多行折行、空序列记录与 `;` 注释行）
pub struct FastaIter {
    data: []const u8,
    pos: usize,
    started: bool = false,

    pub static fn init(data: []const u8): FastaIter {
        return FastaIter {
            data: data,
            pos: skipLeadingTrivia(data),
            started: false,
        };
    }

    pub fn next(*_): FastaRecord? {
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            while (_.pos < _.data.len && _.data[_.pos] != '\n') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.data.len && _.data[_.pos] == '\n') {
                _.pos += 1;
            }

            const hdr_line = trimLine(_.data[line_start..line_end]);
            if (hdr_line.len == 0 || hdr_line[0] == ';') {
                continue;
            }
            if (hdr_line[0] != '>') {
                if (!_.started) {
                    _.pos = _.data.len;
                    return null;
                }
                continue;
            }

            _.started = true;

            // 解析 `>id description`
            const after_gt = trimLine(hdr_line[1..hdr_line.len]);
            let id_end: usize = 0;
            while (id_end < after_gt.len && !isHSpace(after_gt[id_end])) {
                id_end += 1;
            }
            const id = after_gt[0..id_end];
            const desc = trimLine(after_gt[id_end..after_gt.len]);

            // 收集序列行直至下一条 `'>'` 或 EOF
            const seq_start = _.pos;
            let seq_end = seq_start;
            while (_.pos < _.data.len) {
                const cur_line_start = _.pos;
                let scan = cur_line_start;
                while (scan < _.data.len && isHSpace(_.data[scan])) {
                    scan += 1;
                }
                if (scan < _.data.len && _.data[scan] == '>') {
                    break;
                }
                while (_.pos < _.data.len && _.data[_.pos] != '\n') {
                    _.pos += 1;
                }
                if (_.pos < _.data.len && _.data[_.pos] == '\n') {
                    _.pos += 1;
                }
                seq_end = _.pos;
            }

            return FastaRecord {
                id: id,
                description: desc,
                raw_seq: _.data[seq_start..seq_end],
            };
        }
        return null;
    }
}

/// DNA 互补碱基映射（完整支持标准 A/T/U/C/G 与 IUPAC 简并碱基 R/Y/S/W/K/M/B/D/H/V/N，保留大小写）
pub fn dnaComplement(base: u8): u8 {
    if (base == 'A') return 'T';
    if (base == 'T') return 'A';
    if (base == 'U') return 'A';
    if (base == 'C') return 'G';
    if (base == 'G') return 'C';
    if (base == 'R') return 'Y';
    if (base == 'Y') return 'R';
    if (base == 'S') return 'S';
    if (base == 'W') return 'W';
    if (base == 'K') return 'M';
    if (base == 'M') return 'K';
    if (base == 'B') return 'V';
    if (base == 'V') return 'B';
    if (base == 'D') return 'H';
    if (base == 'H') return 'D';
    if (base == 'N') return 'N';

    if (base == 'a') return 't';
    if (base == 't') return 'a';
    if (base == 'u') return 'a';
    if (base == 'c') return 'g';
    if (base == 'g') return 'c';
    if (base == 'r') return 'y';
    if (base == 'y') return 'r';
    if (base == 's') return 's';
    if (base == 'w') return 'w';
    if (base == 'k') return 'm';
    if (base == 'm') return 'k';
    if (base == 'b') return 'v';
    if (base == 'v') return 'b';
    if (base == 'd') return 'h';
    if (base == 'h') return 'd';
    if (base == 'n') return 'n';

    return base;
}

/// 零堆分配计算 DNA 反向互补序列（自动忽略输入空白/换行；缓冲区不足返回 `null`）
pub fn reverseComplementDna(seq: []const u8, out: []u8): usize? {
    let out_len: usize = 0;
    let i = seq.len;
    while (i > 0) {
        i -= 1;
        const c = seq[i];
        if (!isSpace(c)) {
            if (out_len >= out.len) return null;
            out[out_len] = dnaComplement(c);
            out_len += 1;
        }
    }
    return out_len;
}

/// 单遍统计 FASTA 数据集的记录数、总碱基数、最小/最大长度及 GC/AT/N 碱基统计
pub fn summarizeFasta(data: []const u8): FastaStats {
    let stats = FastaStats {};
    if (!isFasta(data)) return stats;

    let it = FastaIter.init(data);
    while (rec := it.next()) {
        let rlen: usize = 0;
        let i: usize = 0;
        while (i < rec.raw_seq.len) {
            const c = rec.raw_seq[i];
            if (!isSpace(c)) {
                rlen += 1;
                if (c == 'G' || c == 'g' || c == 'C' || c == 'c') {
                    stats.gc_bases += 1;
                } else if (c == 'A' || c == 'a' || c == 'T' || c == 't' || c == 'U' || c == 'u') {
                    stats.at_bases += 1;
                } else if (c == 'N' || c == 'n') {
                    stats.n_bases += 1;
                }
            }
            i += 1;
        }

        if (stats.record_count == 0) {
            stats.min_len = rlen;
            stats.max_len = rlen;
        } else {
            if (rlen < stats.min_len) stats.min_len = rlen;
            if (rlen > stats.max_len) stats.max_len = rlen;
        }
        stats.record_count += 1;
        stats.total_bases += rlen;
    }

    return stats;
}
