// Origin: Pure-CP Stdlib (Zero-Alloc Research Information Systems .ris Citation Parser) | Created: 2026-11-22

/// 单条 RIS 标签（2 字节大写标签码 + 零拷贝值切片，支持多行续行）
pub struct RisTag {
    code: [2]u8,
    value: []const u8,

    /// 判断标签码是否匹配 2 字节标签名（如 "TY", "AU", "TI", "KW", "ER"）
    fn isCode(tag_2b: []const u8): bool {
        return tag_2b.len == 2 && _.code[0] == tag_2b[0] && _.code[1] == tag_2b[1];
    }
}

/// 单条 RIS 学术文献引文记录（零拷贝切片）
pub struct RisRecord {
    ref_type: []const u8,
    title: []const u8,
    first_author: []const u8,
    author_count: u32,
    journal: []const u8,
    year: []const u8,
    volume: []const u8,
    issue: []const u8,
    start_page: []const u8,
    end_page: []const u8,
    doi: []const u8,
    url: []const u8,
    abstract_text: []const u8,
    raw_record: []const u8,

    /// 返回该条记录内全部 RIS 标签的零分配迭代器
    fn tags(): RisTagIter {
        return RisTagIter.init(_.raw_record);
    }

    /// 查找首个匹配的 2 字节 RIS 标签值
    fn findTag(tag_2b: []const u8): ([]const u8)? {
        let it = _.tags();
        while (t := it.next()) {
            if (t.isCode(tag_2b)) {
                return t.value;
            }
        }
        return null;
    }
}

/// RIS 文献集单遍统计汇总
pub struct RisStats {
    record_count: u32,
    journal_count: u32,
    book_count: u32,
    conf_count: u32,
    with_doi_count: u32,
    total_authors: u32,
    total_keywords: u32,
}

// ---------------------------------------------------------------------------
// 内部零分配切片与标签识别辅助函数
// ---------------------------------------------------------------------------

fn isUpper(c: u8): bool {
    return c >= 'A' && c <= 'Z';
}

fn isUpperOrDigit(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpaceOrTab(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpaceOrTab(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
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

/// 跳过可选 UTF-8 BOM (EF BB BF)
fn skipBom(data: []const u8): usize {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return 3;
    }
    return 0;
}

/// 从 data[*pos..] 切出下一行（不含 \r\n），并推进 pos
fn nextLine(data: []const u8, pos: *usize): ([]const u8)? {
    const start = pos.*;
    if (start >= data.len) {
        return null;
    }
    let i = start;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    let line_end = i;
    if (line_end > start && data[line_end - 1] == '\r') {
        line_end -= 1;
    }
    pos.* = if (i < data.len) (i + 1) else data.len;
    return data[start..line_end];
}

/// 判断一行是否为标准 RIS 标签行："XX  -"（2 位大写字母/数字 + 2 空格 + '-' + 可选空格）
fn isRisTagLine(line: []const u8): bool {
    if (line.len < 5) {
        return false;
    }
    if (!isUpper(line[0]) || !isUpperOrDigit(line[1])) {
        return false;
    }
    if (line[2] != ' ' || line[3] != ' ' || line[4] != '-') {
        return false;
    }
    if (line.len == 5) {
        return true;
    }
    return line[5] == ' ';
}

fn isTyLine(line: []const u8): bool {
    return isRisTagLine(line) && line[0] == 'T' && line[1] == 'Y';
}

fn isErLine(line: []const u8): bool {
    return isRisTagLine(line) && line[0] == 'E' && line[1] == 'R';
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 快速检测缓冲区是否为 RIS 引文格式（跳过可选 UTF-8 BOM 与空行，检测 "TY  - " 开头）
pub fn isRis(data: []const u8): bool {
    let pos = skipBom(data);
    while (line := nextLine(data, *pos)) {
        if (trimAscii(line).len == 0) {
            continue;
        }
        return line.len >= 6 &&
            line[0] == 'T' &&
            line[1] == 'Y' &&
            line[2] == ' ' &&
            line[3] == ' ' &&
            line[4] == '-' &&
            line[5] == ' ';
    }
    return false;
}

/// 在单条 RIS 记录切片内零分配迭代全部标签（自动合并后续非标签续行）
pub struct RisTagIter {
    data: []const u8,
    pos: usize,
    done: bool,

    fn init(record_slice: []const u8): RisTagIter {
        return RisTagIter{
            data: record_slice,
            pos: skipBom(record_slice),
            done: false,
        };
    }

    fn next(*_): RisTag? {
        if (_.done) {
            return null;
        }

        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const line = nextLine(_.data, *_.pos) ?? break;
            if (!isRisTagLine(line)) {
                continue;
            }

            const code: [2]u8 = [line[0], line[1]];
            let val_start = line_start + 5;
            if (line.len >= 6 && line[5] == ' ') {
                val_start = line_start + 6;
            }
            // 跳过值前多余空格（保持首行整洁）
            while (val_start < line_start + line.len && isSpaceOrTab(_.data[val_start])) {
                val_start += 1;
            }
            let val_end = line_start + line.len;
            while (val_end > val_start && isSpaceOrTab(_.data[val_end - 1])) {
                val_end -= 1;
            }

            if (code[0] == 'E' && code[1] == 'R') {
                _.done = true;
                return RisTag{
                    code: code,
                    value: _.data[val_start..val_end],
                };
            }

            // 合并后续非标签、非空续行至当前标签值（零拷贝切片扩展）
            while (_.pos < _.data.len) {
                const probe_start = _.pos;
                const next_l = nextLine(_.data, *_.pos) ?? break;
                if (isRisTagLine(next_l) || trimAscii(next_l).len == 0) {
                    _.pos = probe_start;
                    break;
                }
                if (val_start == val_end) {
                    val_start = probe_start;
                }
                val_end = probe_start + next_l.len;
            }

            return RisTag{
                code: code,
                value: _.data[val_start..val_end],
            };
        }

        _.done = true;
        return null;
    }
}

/// 解析单条记录切片为 RisRecord
fn parseRisRecord(raw_record: []const u8): RisRecord {
    const empty = raw_record[0..0];
    let rec = RisRecord{
        ref_type: empty,
        title: empty,
        first_author: empty,
        author_count: 0,
        journal: empty,
        year: empty,
        volume: empty,
        issue: empty,
        start_page: empty,
        end_page: empty,
        doi: empty,
        url: empty,
        abstract_text: empty,
        raw_record: raw_record,
    };

    let it = RisTagIter.init(raw_record);
    while (tag := it.next()) {
        if (tag.isCode("TY")) {
            if (rec.ref_type.len == 0) rec.ref_type = tag.value;
        } else if (tag.isCode("TI") || tag.isCode("T1")) {
            if (rec.title.len == 0) rec.title = tag.value;
        } else if (tag.isCode("AU") || tag.isCode("A1")) {
            if (rec.first_author.len == 0) rec.first_author = tag.value;
            rec.author_count += 1;
        } else if (tag.isCode("JO") || tag.isCode("JF") || tag.isCode("T2")) {
            if (rec.journal.len == 0) rec.journal = tag.value;
        } else if (tag.isCode("PY") || tag.isCode("Y1")) {
            if (rec.year.len == 0) rec.year = tag.value;
        } else if (tag.isCode("VL")) {
            if (rec.volume.len == 0) rec.volume = tag.value;
        } else if (tag.isCode("IS")) {
            if (rec.issue.len == 0) rec.issue = tag.value;
        } else if (tag.isCode("SP")) {
            if (rec.start_page.len == 0) rec.start_page = tag.value;
        } else if (tag.isCode("EP")) {
            if (rec.end_page.len == 0) rec.end_page = tag.value;
        } else if (tag.isCode("DO")) {
            if (rec.doi.len == 0) rec.doi = tag.value;
        } else if (tag.isCode("UR")) {
            if (rec.url.len == 0) rec.url = tag.value;
        } else if (tag.isCode("AB") || tag.isCode("N2")) {
            if (rec.abstract_text.len == 0) rec.abstract_text = tag.value;
        }
    }

    return rec;
}

/// 零分配多文献记录迭代器（每条由 "TY  - " 开启、"ER  -" 结束，兼容 CRLF/LF、BOM 及缺 ER 容错）
pub struct RisIter {
    data: []const u8,
    pos: usize,

    fn init(data: []const u8): RisIter {
        return RisIter{
            data: data,
            pos: skipBom(data),
        };
    }

    fn next(*_): RisRecord? {
        // 定位下一个 "TY  -" 起点
        let rec_start: usize = 0;
        let found = false;
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const line = nextLine(_.data, *_.pos) ?? break;
            if (isTyLine(line)) {
                rec_start = line_start;
                found = true;
                break;
            }
        }
        if (!found) {
            return null;
        }

        // 向前扫描至 "ER  -"、下一条 "TY  -"（缺 ER 容错）或 EOF
        let rec_end = _.data.len;
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const line = nextLine(_.data, *_.pos) ?? break;
            if (isErLine(line)) {
                rec_end = _.pos;
                break;
            }
            if (isTyLine(line)) {
                rec_end = line_start;
                _.pos = line_start;
                break;
            }
        }

        return parseRisRecord(_.data[rec_start..rec_end]);
    }
}

/// 单遍统计 RIS 文献库摘要（记录数、期刊/图书/会议数、含 DOI 数、总作者数与总关键词数）
pub fn summarizeRis(data: []const u8): RisStats {
    let stats = RisStats{
        record_count: 0,
        journal_count: 0,
        book_count: 0,
        conf_count: 0,
        with_doi_count: 0,
        total_authors: 0,
        total_keywords: 0,
    };

    let it = RisIter.init(data);
    while (rec := it.next()) {
        stats.record_count += 1;
        if (eqlBytes(rec.ref_type, "JOUR")) {
            stats.journal_count += 1;
        } else if (eqlBytes(rec.ref_type, "BOOK")) {
            stats.book_count += 1;
        } else if (eqlBytes(rec.ref_type, "CONF")) {
            stats.conf_count += 1;
        }
        if (rec.doi.len > 0) {
            stats.with_doi_count += 1;
        }
        stats.total_authors += rec.author_count;

        let tit = rec.tags();
        while (tag := tit.next()) {
            if (tag.isCode("KW")) {
                stats.total_keywords += 1;
            }
        }
    }

    return stats;
}
