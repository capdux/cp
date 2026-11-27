// Origin: Pure-CP Stdlib (Zero-Alloc GEDCOM 5.5.1 / 7.0 Genealogy Parser) | Created: 2026-11-22

/// 单条 GEDCOM 逻辑行：<level> [@xref@] <TAG> [value]（零拷贝切片）
pub struct GedcomLine {
    level: u16,
    xref: []const u8,
    tag: []const u8,
    value: []const u8,

    /// 判断 value 是否为 GEDCOM 交叉引用指针（如 "@I1@"、"@F1@"）
    fn isPointerValue(): bool {
        const s = trimAscii(_.value);
        if (s.len < 3) {
            return false;
        }
        if (s[0] != '@' || s[s.len - 1] != '@') {
            return false;
        }
        // 排除转义符 "@@" 以及内部包含多余 '@' 的普通文本
        let i: usize = 1;
        while (i + 1 < s.len) {
            if (s[i] == '@' || isSpaceOrTab(s[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

/// GEDCOM 0 HEAD 头部元数据（零拷贝切片）
pub struct GedcomHeader {
    gedc_version: []const u8,
    charset: []const u8,
    source_system: []const u8,
    language: []const u8,
}

/// GEDCOM 顶层个人记录（0 @I1@ INDI）
pub struct GedcomIndividual {
    xref: []const u8,
    name: []const u8,
    sex: []const u8,
    birth_date: []const u8,
    birth_place: []const u8,
    death_date: []const u8,
    death_place: []const u8,
    famc_xref: []const u8,
    fams_xref: []const u8,
}

/// GEDCOM 顶层家庭记录（0 @F1@ FAM）
pub struct GedcomFamily {
    xref: []const u8,
    husb_xref: []const u8,
    wife_xref: []const u8,
    marr_date: []const u8,
    child_count: u32,
}

/// GEDCOM 单遍结构统计汇总
pub struct GedcomStats {
    line_count: u32,
    indi_count: u32,
    fam_count: u32,
    sour_count: u32,
    note_count: u32,
    max_level: u16,
    has_trailer: bool,
}

// ---------------------------------------------------------------------------
// 内部零分配切片与行解析辅助函数
// ---------------------------------------------------------------------------

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isTagChar(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
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

fn skipBom(data: []const u8): usize {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return 3;
    }
    return 0;
}

/// 兼容 \r\n、\n 以及单独 \r 的零拷贝逐行切片
fn nextRawLine(data: []const u8, pos: *usize): ([]const u8)? {
    const start = pos.*;
    if (start >= data.len) {
        return null;
    }
    let i = start;
    while (i < data.len && data[i] != '\n' && data[i] != '\r') {
        i += 1;
    }
    const line = data[start..i];
    if (i < data.len) {
        if (data[i] == '\r' && i + 1 < data.len && data[i + 1] == '\n') {
            pos.* = i + 2;
        } else {
            pos.* = i + 1;
        }
    } else {
        pos.* = data.len;
    }
    return line;
}

fn stripLineEnd(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end -= 1;
    }
    return line[0..end];
}

fn isValidXrefToken(tok: []const u8): bool {
    if (tok.len < 3 || tok[0] != '@' || tok[tok.len - 1] != '@') {
        return false;
    }
    let i: usize = 1;
    while (i + 1 < tok.len) {
        if (tok[i] == '@' || isSpaceOrTab(tok[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 公开 API：行解析与流式迭代器
// ---------------------------------------------------------------------------

/// 解析单条 GEDCOM 行：<level> [@xref@] <TAG> [value]
pub fn parseGedcomLine(raw_line: []const u8): GedcomLine? {
    const line = stripLineEnd(raw_line);
    let i: usize = 0;
    while (i < line.len && isSpaceOrTab(line[i])) {
        i += 1;
    }
    if (i >= line.len || !isDigit(line[i])) {
        return null;
    }

    // 1. 解析 <level> (u16)
    let level_acc: u32 = 0;
    while (i < line.len && isDigit(line[i])) {
        level_acc = level_acc * 10 + u32(line[i] - '0');
        if (level_acc > 65535) {
            return null;
        }
        i += 1;
    }

    // 必须至少跟 1 个空格
    if (i >= line.len || !isSpaceOrTab(line[i])) {
        return null;
    }
    while (i < line.len && isSpaceOrTab(line[i])) {
        i += 1;
    }
    if (i >= line.len) {
        return null;
    }

    // 2. 可选 @xref@
    let xref = line[0..0];
    if (line[i] == '@') {
        const x_start = i;
        while (i < line.len && !isSpaceOrTab(line[i])) {
            i += 1;
        }
        const cand = line[x_start..i];
        if (!isValidXrefToken(cand)) {
            return null;
        }
        xref = cand;
        if (i >= line.len || !isSpaceOrTab(line[i])) {
            return null;
        }
        while (i < line.len && isSpaceOrTab(line[i])) {
            i += 1;
        }
        if (i >= line.len) {
            return null;
        }
    }

    // 3. 必填 <TAG>
    const tag_start = i;
    while (i < line.len && !isSpaceOrTab(line[i])) {
        if (!isTagChar(line[i])) {
            return null;
        }
        i += 1;
    }
    const tag = line[tag_start..i];
    if (tag.len == 0) {
        return null;
    }

    // 4. 可选 [value]（跳过 <TAG> 后的单个分隔空格，保留值内前导空格以兼容 CONC/CONT）
    let value = line[line.len..line.len];
    if (i < line.len && isSpaceOrTab(line[i])) {
        i += 1;
        value = line[i..line.len];
    }

    return GedcomLine{
        level: level_acc as u16,
        xref: xref,
        tag: tag,
        value: value,
    };
}

/// 零分配逐行迭代全部 GEDCOM 行（兼容 \r\n/\n/\r，跳过空行，校验 level <= prev_level + 1）
pub struct GedcomLineIter {
    data: []const u8,
    pos: usize,
    prev_level: u16,
    has_prev: bool,
    malformed: bool,

    static fn init(data: []const u8): GedcomLineIter {
        return GedcomLineIter{
            data: data,
            pos: skipBom(data),
            prev_level: 0,
            has_prev: false,
            malformed: false,
        };
    }

    fn next(*_): GedcomLine? {
        if (_.malformed) {
            return null;
        }
        while (raw := nextRawLine(_.data, *_.pos)) {
            if (trimAscii(raw).len == 0) {
                continue;
            }
            if (parsed := parseGedcomLine(raw)) {
                if (_.has_prev && parsed.level > _.prev_level + 1) {
                    _.malformed = true;
                    return null;
                }
                _.prev_level = parsed.level;
                _.has_prev = true;
                return parsed;
            }
            _.malformed = true;
            return null;
        }
        return null;
    }
}

/// 快速检测缓冲区是否以合法的 "0 HEAD" 开头（跳过可选 UTF-8 BOM 与空行）
pub fn isGedcom(data: []const u8): bool {
    let it = GedcomLineIter.init(data);
    const first = it.next() ?? return false;
    return first.level == 0 && first.xref.len == 0 && eqlBytes(first.tag, "HEAD");
}

/// 解析 0 HEAD 头部块，提取 gedc_version (GEDC->VERS)、charset (CHAR)、source_system (SOUR)、language (LANG)
pub fn parseGedcomHeader(data: []const u8): GedcomHeader? {
    let it = GedcomLineIter.init(data);
    const first = it.next() ?? return null;
    if (first.level != 0 || !eqlBytes(first.tag, "HEAD")) {
        return null;
    }

    const empty = data[0..0];
    let hdr = GedcomHeader{
        gedc_version: empty,
        charset: empty,
        source_system: empty,
        language: empty,
    };

    let in_gedc = false;
    while (line := it.next()) {
        if (line.level == 0) {
            break;
        }
        if (line.level == 1) {
            in_gedc = eqlBytes(line.tag, "GEDC");
            if (eqlBytes(line.tag, "CHAR")) {
                if (hdr.charset.len == 0) hdr.charset = trimAscii(line.value);
            } else if (eqlBytes(line.tag, "SOUR")) {
                if (hdr.source_system.len == 0) hdr.source_system = trimAscii(line.value);
            } else if (eqlBytes(line.tag, "LANG")) {
                if (hdr.language.len == 0) hdr.language = trimAscii(line.value);
            }
        } else if (line.level == 2 && in_gedc && eqlBytes(line.tag, "VERS")) {
            if (hdr.gedc_version.len == 0) hdr.gedc_version = trimAscii(line.value);
        }
    }

    return hdr;
}

// ---------------------------------------------------------------------------
// 公开 API：顶层实体记录（INDI / FAM）零分配迭代器
// ---------------------------------------------------------------------------

/// 零分配迭代全部顶层个人记录（0 @I...@ INDI）
pub struct GedcomIndiIter {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): GedcomIndiIter {
        return GedcomIndiIter{
            data: data,
            pos: skipBom(data),
        };
    }

    fn next(*_): GedcomIndividual? {
        // 寻找下一条 "0 @...@ INDI"
        let indi_xref = _.data[0..0];
        let found = false;
        while (raw := nextRawLine(_.data, *_.pos)) {
            if (trimAscii(raw).len == 0) {
                continue;
            }
            if (line := parseGedcomLine(raw)) {
                if (line.level == 0 && eqlBytes(line.tag, "INDI")) {
                    indi_xref = line.xref;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            return null;
        }

        const empty = _.data[0..0];
        let rec = GedcomIndividual{
            xref: indi_xref,
            name: empty,
            sex: empty,
            birth_date: empty,
            birth_place: empty,
            death_date: empty,
            death_place: empty,
            famc_xref: empty,
            fams_xref: empty,
        };

        // sub_ctx: 0=none, 1=BIRT, 2=DEAT
        let sub_ctx: u8 = 0;
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const raw = nextRawLine(_.data, *_.pos) ?? break;
            if (trimAscii(raw).len == 0) {
                continue;
            }
            const line = parseGedcomLine(raw) ?? continue;
            if (line.level == 0) {
                _.pos = line_start;
                break;
            }
            if (line.level == 1) {
                if (eqlBytes(line.tag, "BIRT")) {
                    sub_ctx = 1;
                } else if (eqlBytes(line.tag, "DEAT")) {
                    sub_ctx = 2;
                } else {
                    sub_ctx = 0;
                    if (eqlBytes(line.tag, "NAME")) {
                        if (rec.name.len == 0) rec.name = trimAscii(line.value);
                    } else if (eqlBytes(line.tag, "SEX")) {
                        if (rec.sex.len == 0) rec.sex = trimAscii(line.value);
                    } else if (eqlBytes(line.tag, "FAMC")) {
                        if (rec.famc_xref.len == 0) rec.famc_xref = trimAscii(line.value);
                    } else if (eqlBytes(line.tag, "FAMS")) {
                        if (rec.fams_xref.len == 0) rec.fams_xref = trimAscii(line.value);
                    }
                }
            } else if (line.level == 2) {
                if (sub_ctx == 1) {
                    if (eqlBytes(line.tag, "DATE")) {
                        if (rec.birth_date.len == 0) rec.birth_date = trimAscii(line.value);
                    } else if (eqlBytes(line.tag, "PLAC")) {
                        if (rec.birth_place.len == 0) rec.birth_place = trimAscii(line.value);
                    }
                } else if (sub_ctx == 2) {
                    if (eqlBytes(line.tag, "DATE")) {
                        if (rec.death_date.len == 0) rec.death_date = trimAscii(line.value);
                    } else if (eqlBytes(line.tag, "PLAC")) {
                        if (rec.death_place.len == 0) rec.death_place = trimAscii(line.value);
                    }
                }
            }
        }

        return rec;
    }
}

/// 零分配迭代全部顶层家庭记录（0 @F...@ FAM）
pub struct GedcomFamIter {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): GedcomFamIter {
        return GedcomFamIter{
            data: data,
            pos: skipBom(data),
        };
    }

    fn next(*_): GedcomFamily? {
        let fam_xref = _.data[0..0];
        let found = false;
        while (raw := nextRawLine(_.data, *_.pos)) {
            if (trimAscii(raw).len == 0) {
                continue;
            }
            if (line := parseGedcomLine(raw)) {
                if (line.level == 0 && eqlBytes(line.tag, "FAM")) {
                    fam_xref = line.xref;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            return null;
        }

        const empty = _.data[0..0];
        let fam = GedcomFamily{
            xref: fam_xref,
            husb_xref: empty,
            wife_xref: empty,
            marr_date: empty,
            child_count: 0,
        };

        let in_marr = false;
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const raw = nextRawLine(_.data, *_.pos) ?? break;
            if (trimAscii(raw).len == 0) {
                continue;
            }
            const line = parseGedcomLine(raw) ?? continue;
            if (line.level == 0) {
                _.pos = line_start;
                break;
            }
            if (line.level == 1) {
                in_marr = eqlBytes(line.tag, "MARR");
                if (eqlBytes(line.tag, "HUSB")) {
                    if (fam.husb_xref.len == 0) fam.husb_xref = trimAscii(line.value);
                } else if (eqlBytes(line.tag, "WIFE")) {
                    if (fam.wife_xref.len == 0) fam.wife_xref = trimAscii(line.value);
                } else if (eqlBytes(line.tag, "CHIL")) {
                    fam.child_count += 1;
                }
            } else if (line.level == 2 && in_marr && eqlBytes(line.tag, "DATE")) {
                if (fam.marr_date.len == 0) fam.marr_date = trimAscii(line.value);
            }
        }

        return fam;
    }
}

/// 单遍统计 GEDCOM 文档摘要（行数、INDI/FAM/SOUR/NOTE 顶层实体数、最大层级与 0 TRLR 标志）
pub fn summarizeGedcom(data: []const u8): GedcomStats {
    let stats = GedcomStats{
        line_count: 0,
        indi_count: 0,
        fam_count: 0,
        sour_count: 0,
        note_count: 0,
        max_level: 0,
        has_trailer: false,
    };

    let it = GedcomLineIter.init(data);
    while (line := it.next()) {
        stats.line_count += 1;
        if (line.level > stats.max_level) {
            stats.max_level = line.level;
        }
        if (line.level == 0) {
            if (eqlBytes(line.tag, "INDI")) {
                stats.indi_count += 1;
            } else if (eqlBytes(line.tag, "FAM")) {
                stats.fam_count += 1;
            } else if (eqlBytes(line.tag, "SOUR")) {
                stats.sour_count += 1;
            } else if (eqlBytes(line.tag, "NOTE")) {
                stats.note_count += 1;
            } else if (eqlBytes(line.tag, "TRLR")) {
                stats.has_trailer = true;
            }
        }
    }

    return stats;
}
