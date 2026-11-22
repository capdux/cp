// Origin: Pure-CP Stdlib (Zero-Alloc HAProxy Config Parser) | Created: 2026-11-22

/// HAProxy 配置段类型
pub enum HaSectionKind {
    global,
    defaults,
    frontend,
    backend,
    listen,
    resolvers,
    peers,
    userlist,
    other,
}

/// HAProxy 配置段（所有切片均零拷贝指向原始输入）
pub struct HaSection {
    kind: HaSectionKind,
    name: ([]const u8)?,
    body: []const u8,
}

/// HAProxy 段内单行指令（已去除行尾 `#` 注释与首尾空白）
pub struct HaDirective {
    keyword: []const u8,
    args: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isHoriSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

/// 剥离单/双引号外且非 `\#` 转义的行尾 `#` 注释
fn stripHashComment(line: []const u8): []const u8 {
    let i: usize = 0;
    let in_single = false;
    let in_double = false;
    while (i < line.len) {
        const c = line[i];
        if (c == '\\' && i + 1 < line.len) {
            i += 2;
            continue;
        }
        if (!in_double && c == '\'') {
            in_single = !in_single;
            i += 1;
            continue;
        }
        if (!in_single && c == '"') {
            in_double = !in_double;
            i += 1;
            continue;
        }
        if (!in_single && !in_double && c == '#') {
            return line[0..i];
        }
        i += 1;
    }
    return line;
}

/// 识别 HAProxy 段头关键字；非段头返回 `null`
fn matchSectionKind(kw: []const u8): HaSectionKind? {
    if (eqlBytes(kw, "global")) return HaSectionKind.global;
    if (eqlBytes(kw, "defaults")) return HaSectionKind.defaults;
    if (eqlBytes(kw, "frontend")) return HaSectionKind.frontend;
    if (eqlBytes(kw, "backend")) return HaSectionKind.backend;
    if (eqlBytes(kw, "listen")) return HaSectionKind.listen;
    if (eqlBytes(kw, "resolvers")) return HaSectionKind.resolvers;
    if (eqlBytes(kw, "peers")) return HaSectionKind.peers;
    if (eqlBytes(kw, "userlist")) return HaSectionKind.userlist;
    if (eqlBytes(kw, "mailers") || eqlBytes(kw, "cache") || eqlBytes(kw, "ring")
        || eqlBytes(kw, "program") || eqlBytes(kw, "http-errors")
        || eqlBytes(kw, "fcgi-app") || eqlBytes(kw, "crt-store")
        || eqlBytes(kw, "traces")) {
        return HaSectionKind.other;
    }
    return null;
}

struct ParsedHeader {
    kind: HaSectionKind,
    name: ([]const u8)?,
}

/// 尝试将单行解析为 HAProxy 段头
fn parseSectionHeader(raw_line: []const u8): ParsedHeader? {
    const clean = trimAscii(stripHashComment(raw_line));
    if (clean.len == 0) return null;

    let kw_end: usize = 0;
    while (kw_end < clean.len && !isHoriSpace(clean[kw_end])) {
        kw_end += 1;
    }
    const kw = clean[0..kw_end];

    let kind = HaSectionKind.other;
    if (k := matchSectionKind(kw)) {
        kind = k;
    } else {
        return null;
    }

    let rest_start = kw_end;
    while (rest_start < clean.len && isHoriSpace(clean[rest_start])) {
        rest_start += 1;
    }
    let name: ([]const u8)? = null;
    if (rest_start < clean.len) {
        let name_end = rest_start;
        while (name_end < clean.len && !isHoriSpace(clean[name_end])) {
            name_end += 1;
        }
        name = clean[rest_start..name_end];
    }

    return ParsedHeader {
        kind: kind,
        name: name,
    };
}

struct LineSpan {
    line: []const u8,
    next_pos: usize,
}

fn nextLine(s: []const u8, pos: usize): LineSpan? {
    if (pos >= s.len) return null;
    let i = pos;
    while (i < s.len && s[i] != '\n') {
        i += 1;
    }
    let line_end = i;
    if (line_end > pos && s[line_end - 1] == '\r') {
        line_end -= 1;
    }
    const next_pos: usize = if (i < s.len) i + 1 else s.len;
    return LineSpan {
        line: s[pos..line_end],
        next_pos: next_pos,
    };
}

/// 零堆分配 HAProxy 配置段迭代器
pub struct HaSectionIter {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): HaSectionIter {
        return HaSectionIter {
            input: input,
            pos: 0,
        };
    }

    /// 读取下一段 `HaSection`；到达末尾返回 `null`
    fn next(*_): HaSection? {
        // 1. 定位下一个段头行
        let header = ParsedHeader {
            kind: HaSectionKind.other,
            name: null,
        };
        let found_header = false;
        while (span := nextLine(_.input, _.pos)) {
            _.pos = span.next_pos;
            if (h := parseSectionHeader(span.line)) {
                header = h;
                found_header = true;
                break;
            }
        }
        if (!found_header) {
            return null;
        }

        // 2. 收集段体，直到遇到下一个段头或 EOF
        const body_start = _.pos;
        let body_end = _.pos;
        while (span := nextLine(_.input, _.pos)) {
            if (parseSectionHeader(span.line) != null) {
                // 不消耗下一段的段头行，留给下一次 next()
                break;
            }
            _.pos = span.next_pos;
            body_end = _.pos;
        }

        // 裁剪段尾多余空白/换行
        while (body_end > body_start && isSpace(_.input[body_end - 1])) {
            body_end -= 1;
        }

        return HaSection {
            kind: header.kind,
            name: header.name,
            body: _.input[body_start..body_end],
        };
    }
}

/// 零堆分配 HAProxy 段内指令逐行迭代器（剥离单/双引号外 `#` 行尾注释）
pub struct HaDirectiveIter {
    body: []const u8,
    pos: usize = 0,

    static fn init(section_body: []const u8): HaDirectiveIter {
        return HaDirectiveIter {
            body: section_body,
            pos: 0,
        };
    }

    /// 读取下一条段内指令；遍历结束返回 `null`
    fn next(*_): HaDirective? {
        while (span := nextLine(_.body, _.pos)) {
            _.pos = span.next_pos;
            const clean = trimAscii(stripHashComment(span.line));
            if (clean.len == 0) {
                continue;
            }

            let kw_end: usize = 0;
            while (kw_end < clean.len && !isHoriSpace(clean[kw_end])) {
                kw_end += 1;
            }
            const keyword = clean[0..kw_end];
            const args = trimAscii(clean[kw_end..]);
            return HaDirective {
                keyword: keyword,
                args: args,
            };
        }
        return null;
    }
}

/// 按段类型与可选段名查找首个匹配的 `HaSection`（`name == null` 时匹配同类型首个段）
pub fn findHaSection(input: []const u8, kind: HaSectionKind, name: ([]const u8)?): HaSection? {
    let it = HaSectionIter.init(input);
    while (sec := it.next()) {
        if (sec.kind != kind) {
            continue;
        }
        if (target := name) {
            if (sec_name := sec.name) {
                if (eqlBytes(sec_name, target)) {
                    return sec;
                }
            }
        } else {
            return sec;
        }
    }
    return null;
}

/// 在段体内按指令关键字查找首个匹配的参数切片（零堆分配）
pub fn findHaDirective(section_body: []const u8, keyword: []const u8): ([]const u8)? {
    let it = HaDirectiveIter.init(section_body);
    while (d := it.next()) {
        if (eqlBytes(d.keyword, keyword)) {
            return d.args;
        }
    }
    return null;
}
