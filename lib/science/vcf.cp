// Origin: Pure-CP Stdlib (Zero-Alloc HTSlib VCF 4.2/4.3 Variant Call Format Parser) | Created: 2026-11-22

/// VCF 文件头解析结果（零拷贝切片）
pub struct VcfHeader {
    version: []const u8,
    reference: []const u8,
    meta_line_count: u32,
    sample_count: u32,
    records_offset: usize,
}

/// 单条 VCF 变异记录（8 核心必选列 + 可选 FORMAT/样本切片，零拷贝）
pub struct VcfRecord {
    chrom: []const u8,
    pos: u64,
    id: []const u8,
    ref_allele: []const u8,
    alt_allele: []const u8,
    qual: f64,
    has_qual: bool,
    filter: []const u8,
    info: []const u8,
    rest_samples: []const u8,

    /// FILTER 为 "PASS" 或 "." 时返回 true
    pub fn isPass(): bool {
        return eqlBytes(_.filter, "PASS") || eqlBytes(_.filter, ".");
    }

    /// 检测是否为单碱基多态性 SNP（ref.len == 1 && alt.len == 1 && 非 "." 且碱基不同）
    pub fn isSnp(): bool {
        if (_.ref_allele.len != 1 || _.alt_allele.len != 1) {
            return false;
        }
        const r = toUpperAscii(_.ref_allele[0]);
        const a = toUpperAscii(_.alt_allele[0]);
        if (!isDnaBase(r) || !isDnaBase(a)) {
            return false;
        }
        return r != a;
    }

    /// 检测是否为插入/缺失变异 Indel（任意非 "." 等位基因长度与 REF 不等）
    pub fn isIndel(): bool {
        if (_.ref_allele.len == 0 || _.alt_allele.len == 0 || eqlBytes(_.alt_allele, ".")) {
            return false;
        }
        let start: usize = 0;
        let i: usize = 0;
        while (i <= _.alt_allele.len) {
            if (i == _.alt_allele.len || _.alt_allele[i] == ',') {
                const allele = _.alt_allele[start..i];
                if (allele.len > 0 && !eqlBytes(allele, ".") && allele.len != _.ref_allele.len) {
                    return true;
                }
                start = i + 1;
            }
            i += 1;
        }
        return false;
    }

    /// 检测 SNP 是否为转换 Transition（A<->G 或 C<->T，大小写不敏感）
    pub fn isTransition(): bool {
        if (!_.isSnp()) {
            return false;
        }
        const r = toUpperAscii(_.ref_allele[0]);
        const a = toUpperAscii(_.alt_allele[0]);
        return (r == 'A' && a == 'G') || (r == 'G' && a == 'A') ||
            (r == 'C' && a == 'T') || (r == 'T' && a == 'C');
    }

    /// 检测 SNP 是否为颠换 Transversion
    pub fn isTransversion(): bool {
        return _.isSnp() && !_.isTransition();
    }

    /// 在分号分隔的 INFO 列中零分配查找 `KEY=VAL`（布尔 Flag `KEY` 返回 `""`，未命中返回 `null`）
    pub fn findInfo(key: []const u8): ([]const u8)? {
        if (key.len == 0 || _.info.len == 0 || eqlBytes(_.info, ".")) {
            return null;
        }
        let start: usize = 0;
        let i: usize = 0;
        while (i <= _.info.len) {
            if (i == _.info.len || _.info[i] == ';') {
                const field = _.info[start..i];
                if (field.len > 0) {
                    let eq_pos: usize? = null;
                    let j: usize = 0;
                    while (j < field.len) {
                        if (field[j] == '=') {
                            eq_pos = j;
                            break;
                        }
                        j += 1;
                    }
                    if (ep := eq_pos) {
                        if (eqlBytes(field[0..ep], key)) {
                            return field[ep + 1 ..];
                        }
                    } else {
                        if (eqlBytes(field, key)) {
                            return "";
                        }
                    }
                }
                start = i + 1;
            }
            i += 1;
        }
        return null;
    }
}

/// VCF 变异单遍统计汇总（含 Ti/Tv 转换颠换比）
pub struct VcfStats {
    record_count: u64,
    pass_count: u64,
    snp_count: u64,
    indel_count: u64,
    transitions: u64,
    transversions: u64,

    pub fn tiTvRatio(): f64 {
        if (_.transversions == 0) {
            return 0.0;
        }
        return f64(_.transitions) / f64(_.transversions);
    }
}

// ---------------------------------------------------------------------------
// 内部零分配基础辅助函数
// ---------------------------------------------------------------------------

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

fn isDnaBase(c: u8): bool {
    return c == 'A' || c == 'C' || c == 'G' || c == 'T';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return eqlBytes(s[0..prefix.len], prefix);
}

fn parseU64(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        acc = acc * 10 + u64(s[i] - '0');
        i += 1;
    }
    return acc;
}

fn parseF64(s: []const u8): f64? {
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    let int_part: f64 = 0.0;
    let digits: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        int_part = int_part * 10.0 + f64(s[i] - '0');
        digits += 1;
        i += 1;
    }
    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && isDigit(s[i])) {
            frac_scale *= 0.1;
            frac_part += f64(s[i] - '0') * frac_scale;
            digits += 1;
            i += 1;
        }
    }
    if (digits == 0) {
        return null;
    }
    let val = int_part + frac_part;
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        let exp_val: u32 = 0;
        let exp_digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + u32(s[i] - '0');
            }
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0 || exp_val > 308) {
            return null;
        }
        let e: u32 = 0;
        while (e < exp_val) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
            e += 1;
        }
    }
    if (i != s.len) {
        return null;
    }
    return if (neg) -val else val;
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

fn countTabColumns(line: []const u8): u32 {
    if (line.len == 0) {
        return 0;
    }
    let cols: u32 = 1;
    let i: usize = 0;
    while (i < line.len) {
        if (line[i] == '\t') {
            cols += 1;
        }
        i += 1;
    }
    return cols;
}

struct TabField {
    slice: []const u8,
    next_pos: usize,
    has_more: bool,
}

fn nextTabField(line: []const u8, pos: usize): TabField? {
    if (pos > line.len) {
        return null;
    }
    let end = pos;
    while (end < line.len && line[end] != '\t') {
        end += 1;
    }
    const has_more = end < line.len && line[end] == '\t';
    return TabField {
        slice: line[pos..end],
        next_pos: if (has_more) (end + 1) else (line.len + 1),
        has_more: has_more,
    };
}

// ---------------------------------------------------------------------------
// 1. 格式探测与文件头解析
// ---------------------------------------------------------------------------

/// 校验首行是否以 `"##fileformat=VCFv"` 开头
pub fn isVcf(data: []const u8): bool {
    const first = readLine(data, 0) ?? return false;
    return startsWith(first.text, "##fileformat=VCFv");
}

/// 零分配解析 VCF 文件头（版本、参考基因组、元信息行数、样本数与记录区起始偏移）
pub fn parseVcfHeader(data: []const u8): VcfHeader? {
    const first = readLine(data, 0) ?? return null;
    const fmt_prefix = "##fileformat=VCFv";
    if (!startsWith(first.text, fmt_prefix)) {
        return null;
    }
    const version = first.text[fmt_prefix.len..];
    if (version.len == 0) {
        return null;
    }

    let reference: []const u8 = "";
    let meta_line_count: u32 = 1;
    let pos = first.next_pos;

    while (lv := readLine(data, pos)) {
        pos = lv.next_pos;
        const line = lv.text;
        if (line.len == 0) {
            continue;
        }
        if (startsWith(line, "##")) {
            meta_line_count += 1;
            const ref_prefix = "##reference=";
            if (reference.len == 0 && startsWith(line, ref_prefix)) {
                reference = line[ref_prefix.len..];
            }
            continue;
        }
        if (startsWith(line, "#CHROM\t")) {
            const cols = countTabColumns(line);
            if (cols < 8) {
                return null;
            }
            const sample_count: u32 = if (cols > 9) (cols - 9) else 0;
            return VcfHeader {
                version: version,
                reference: reference,
                meta_line_count: meta_line_count,
                sample_count: sample_count,
                records_offset: pos,
            };
        }
        // 在 #CHROM 出现前遇到非法非注释行
        return null;
    }

    return null;
}

// ---------------------------------------------------------------------------
// 2. 变异记录零分配迭代器
// ---------------------------------------------------------------------------

/// 零分配逐行迭代 VCF 变异记录（自动跳过 `##` 与 `#CHROM` 头行及空行，兼容 `\r\n`/`\n`）
pub struct VcfRecordIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    pub static fn init(data: []const u8): VcfRecordIter {
        return VcfRecordIter {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    fn fail(*_): VcfRecord? {
        _.had_error = true;
        _.pos = _.data.len;
        return null;
    }

    pub fn next(*_): VcfRecord? {
        while (lv := readLine(_.data, _.pos)) {
            _.pos = lv.next_pos;
            const line = lv.text;
            if (line.len == 0 || line[0] == '#') {
                continue;
            }

            const f0 = nextTabField(line, 0) ?? return _.fail();
            if (!f0.has_more) return _.fail();
            const f1 = nextTabField(line, f0.next_pos) ?? return _.fail();
            if (!f1.has_more) return _.fail();
            const f2 = nextTabField(line, f1.next_pos) ?? return _.fail();
            if (!f2.has_more) return _.fail();
            const f3 = nextTabField(line, f2.next_pos) ?? return _.fail();
            if (!f3.has_more) return _.fail();
            const f4 = nextTabField(line, f3.next_pos) ?? return _.fail();
            if (!f4.has_more) return _.fail();
            const f5 = nextTabField(line, f4.next_pos) ?? return _.fail();
            if (!f5.has_more) return _.fail();
            const f6 = nextTabField(line, f5.next_pos) ?? return _.fail();
            if (!f6.has_more) return _.fail();
            const f7 = nextTabField(line, f6.next_pos) ?? return _.fail();

            const pos_val = parseU64(f1.slice) ?? return _.fail();
            if (pos_val == 0) {
                return _.fail();
            }

            let qual: f64 = 0.0;
            let has_qual = false;
            if (!eqlBytes(f5.slice, ".")) {
                qual = parseF64(f5.slice) ?? return _.fail();
                has_qual = true;
            }

            const rest_samples: []const u8 = if (f7.has_more && f7.next_pos <= line.len) line[f7.next_pos..] else "";

            return VcfRecord {
                chrom: f0.slice,
                pos: pos_val,
                id: f2.slice,
                ref_allele: f3.slice,
                alt_allele: f4.slice,
                qual: qual,
                has_qual: has_qual,
                filter: f6.slice,
                info: f7.slice,
                rest_samples: rest_samples,
            };
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// 3. 单遍变异统计汇总 (summarizeVcf)
// ---------------------------------------------------------------------------

/// 单遍扫描整份 VCF 数据，统计总记录数、PASS 数、SNP/Indel 数及 Ti/Tv 转换颠换比
pub fn summarizeVcf(data: []const u8): VcfStats {
    let stats = VcfStats {
        record_count: 0,
        pass_count: 0,
        snp_count: 0,
        indel_count: 0,
        transitions: 0,
        transversions: 0,
    };

    let it = VcfRecordIter.init(data);
    while (rec := it.next()) {
        stats.record_count += 1;
        if (rec.isPass()) stats.pass_count += 1;
        if (rec.isSnp()) {
            stats.snp_count += 1;
            if (rec.isTransition()) {
                stats.transitions += 1;
            } else if (rec.isTransversion()) {
                stats.transversions += 1;
            }
        } else if (rec.isIndel()) {
            stats.indel_count += 1;
        }
    }

    return stats;
}
