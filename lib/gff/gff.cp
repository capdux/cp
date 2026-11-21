// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc GFF3 & GTF2 Genomic Feature Annotation Parser)

/// GFF3 / GTF2 缺失相位常量 (`"."`)
pub const GFF_PHASE_NONE: u8 = 255;

/// GFF3 / GTF2 9 列基因特征零拷贝记录（1-based 闭区间 `[start, end]`）
pub struct GffRecord {
    seqid: []const u8,
    source: []const u8,
    feature_type: []const u8,
    start: u64,
    end: u64,
    score: f64 = 0.0,
    has_score: bool = false,
    strand: u8 = '.',
    phase: u8 = GFF_PHASE_NONE,
    attributes: []const u8 = "",

    /// 1-based 闭区间碱基长度 (`end - start + 1`)
    pub fn length(): u64 {
        return _.end - _.start + 1;
    }

    /// 零拷贝查找属性键值（同时兼容 GFF3 `key=val;` 与 GTF2 `key "val";` 并自动去引号）
    pub fn findAttr(key: []const u8): ([]const u8)? {
        if (key.len == 0 || _.attributes.len == 0) return null;

        let pos: usize = 0;
        while (pos < _.attributes.len) {
            while (pos < _.attributes.len && (isHSpace(_.attributes[pos]) || _.attributes[pos] == ';')) {
                pos += 1;
            }
            if (pos >= _.attributes.len) break;

            // 扫描单条属性至未加引号的 `;`
            const item_start = pos;
            let in_quote = false;
            while (pos < _.attributes.len) {
                const c = _.attributes[pos];
                if (c == '"') {
                    in_quote = !in_quote;
                } else if (!in_quote && c == ';') {
                    break;
                }
                pos += 1;
            }
            const item = trimHSpace(_.attributes[item_start..pos]);
            if (pos < _.attributes.len && _.attributes[pos] == ';') {
                pos += 1;
            }
            if (item.len == 0) continue;

            // 查找首个 `=` 或空白分隔符
            let sep: usize = 0;
            while (sep < item.len && item[sep] != '=' && !isHSpace(item[sep])) {
                sep += 1;
            }
            if (sep == 0 || sep >= item.len) continue;

            const k = item[0..sep];
            if (!strEql(k, key)) continue;

            // GFF3 `key=val` 或 GTF2 `key "val"`
            let val_start = sep;
            if (item[sep] == '=') {
                val_start = sep + 1;
            } else {
                while (val_start < item.len && isHSpace(item[val_start])) {
                    val_start += 1;
                }
                if (val_start < item.len && item[val_start] == '=') {
                    val_start += 1;
                }
            }

            let val = trimHSpace(item[val_start..item.len]);
            if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                val = val[1 .. val.len - 1];
            }
            return val;
        }
        return null;
    }
}

/// GFF3 / GTF2 数据集统计汇总
pub struct GffStats {
    record_count: usize = 0,
    gene_count: usize = 0,
    exon_count: usize = 0,
    cds_count: usize = 0,
    total_feature_bp: u64 = 0,
    plus_count: usize = 0,
    minus_count: usize = 0,
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

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (haystack[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn trimHSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isHSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn stripCrlf(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[0..end];
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

fn pow10Pos(exp: u32): f64 {
    let result: f64 = 1.0;
    let base: f64 = 10.0;
    let e = exp;
    while (e > 0) {
        if ((e & 1) != 0) result = result * base;
        base = base * base;
        e = e >> 1;
    }
    return result;
}

fn parseF64(tok: []const u8): f64? {
    if (tok.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (tok[i] == '-') {
        neg = true;
        i += 1;
    } else if (tok[i] == '+') {
        i += 1;
    }

    let val: f64 = 0.0;
    let digits: usize = 0;
    while (i < tok.len && tok[i] >= '0' && tok[i] <= '9') {
        val = val * 10.0 + ((tok[i] - '0') as f64);
        digits += 1;
        i += 1;
    }
    if (i < tok.len && tok[i] == '.') {
        i += 1;
        let frac: f64 = 0.0;
        let scale: f64 = 0.1;
        while (i < tok.len && tok[i] >= '0' && tok[i] <= '9') {
            frac = frac + ((tok[i] - '0') as f64) * scale;
            scale = scale * 0.1;
            digits += 1;
            i += 1;
        }
        val = val + frac;
    }
    if (digits == 0) return null;

    if (i < tok.len && (tok[i] == 'e' || tok[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < tok.len && tok[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < tok.len && tok[i] == '+') {
            i += 1;
        }
        let exp_digits: usize = 0;
        let exp_val: u32 = 0;
        while (i < tok.len && tok[i] >= '0' && tok[i] <= '9') {
            if (exp_val < 308) {
                exp_val = exp_val * 10 + u32(tok[i] - '0');
            }
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0) return null;
        const factor = pow10Pos(exp_val);
        if (exp_neg) val = val / factor else val = val * factor;
    }
    if (i != tok.len) return null;
    return if (neg) -val else val;
}

/// 校验首行是否以 `"##gff-version 3"` 开头（自动跳过可选 UTF-8 BOM）
pub fn isGff3(data: []const u8): bool {
    let start: usize = 0;
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        start = 3;
    }
    return startsWith(data[start..data.len], "##gff-version 3");
}

/// 解析单行 9 列 Tab 分隔 GFF3 / GTF2 记录（严格校验 `start >= 1 && start <= end`、strand 与 phase）
pub fn parseGffLine(line: []const u8): GffRecord? {
    const s = stripCrlf(line);
    if (s.len == 0) return null;

    let cols: [9][]const u8 = undefined;
    let cidx: usize = 0;
    let cstart: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\t') {
            if (cidx >= 8) return null;
            cols[cidx] = s[cstart..i];
            cidx += 1;
            cstart = i + 1;
        }
        i += 1;
    }
    if (cidx != 8) return null;
    cols[8] = s[cstart..s.len];

    if (cols[0].len == 0 || cols[2].len == 0) return null;

    const start_pos = parseU64Dec(cols[3]) ?? return null;
    const end_pos = parseU64Dec(cols[4]) ?? return null;
    if (start_pos == 0 || start_pos > end_pos) return null;

    let score: f64 = 0.0;
    let has_score = false;
    if (!strEql(cols[5], ".")) {
        score = parseF64(cols[5]) ?? return null;
        has_score = true;
    }

    if (cols[6].len != 1) return null;
    const strand = cols[6][0];
    if (strand != '+' && strand != '-' && strand != '.' && strand != '?') {
        return null;
    }

    if (cols[7].len != 1) return null;
    const pc = cols[7][0];
    let phase: u8 = GFF_PHASE_NONE;
    if (pc == '0') {
        phase = 0;
    } else if (pc == '1') {
        phase = 1;
    } else if (pc == '2') {
        phase = 2;
    } else if (pc == '.') {
        phase = GFF_PHASE_NONE;
    } else {
        return null;
    }

    return GffRecord {
        seqid: cols[0],
        source: cols[1],
        feature_type: cols[2],
        start: start_pos,
        end: end_pos,
        score: score,
        has_score: has_score,
        strand: strand,
        phase: phase,
        attributes: cols[8],
    };
}

/// 零堆分配 GFF3 / GTF2 流式迭代器（自动跳过空行与 `#`/`##` 注释行，遇 `##FASTA` 立即停止）
pub struct GffIter {
    data: []const u8,
    pos: usize = 0,
    reached_fasta: bool = false,
    had_error: bool = false,

    pub static fn init(data: []const u8): GffIter {
        let start: usize = 0;
        if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
            start = 3;
        }
        return GffIter {
            data: data,
            pos: start,
            reached_fasta: false,
            had_error: false,
        };
    }

    pub fn next(*_): GffRecord? {
        if (_.reached_fasta || _.had_error) return null;

        while (_.pos < _.data.len) {
            const line_start = _.pos;
            while (_.pos < _.data.len && _.data[_.pos] != '\n') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.data.len && _.data[_.pos] == '\n') {
                _.pos += 1;
            }

            const raw = stripCrlf(_.data[line_start..line_end]);
            const trimmed = trimHSpace(raw);
            if (trimmed.len == 0) continue;

            if (trimmed[0] == '#') {
                if (strEql(trimmed, "##FASTA") || startsWith(trimmed, "##FASTA ") || startsWith(trimmed, "##FASTA\t")) {
                    _.reached_fasta = true;
                    return null;
                }
                continue;
            }

            const rec_opt = parseGffLine(raw);
            if (rec_opt == null) {
                _.had_error = true;
                return null;
            }
            return rec_opt;
        }
        return null;
    }
}

/// 单遍统计 GFF3 / GTF2 特征记录数、gene/exon/CDS 计数、总特征碱基跨度与正负链统计
pub fn summarizeGff(data: []const u8): GffStats {
    let stats = GffStats {};
    let it = GffIter.init(data);

    while (rec := it.next()) {
        stats.record_count += 1;
        stats.total_feature_bp += rec.length();
        if (strEql(rec.feature_type, "gene")) {
            stats.gene_count += 1;
        } else if (strEql(rec.feature_type, "exon")) {
            stats.exon_count += 1;
        } else if (strEql(rec.feature_type, "CDS")) {
            stats.cds_count += 1;
        }
        if (rec.strand == '+') {
            stats.plus_count += 1;
        } else if (rec.strand == '-') {
            stats.minus_count += 1;
        }
    }

    return stats;
}
