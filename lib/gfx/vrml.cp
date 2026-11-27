// Origin: Pure-CP Stdlib (Zero-Alloc VRML97 & Classic X3D Scene Parser) | Created: 2026-11-22

/// VRML / Classic X3D 方言类型
pub enum VrmlFlavor: u8 {
    vrml97 = 0,
    x3d_classic = 1,
    unknown = 255,
}

/// VRML 词法单元类型
pub enum VrmlTokenKind: u8 {
    ident = 0,
    number = 1,
    string = 2,
    lbrace = 3,
    rbrace = 4,
    lbracket = 5,
    rbracket = 6,
}

/// VRML / Classic X3D 文件头解析结果（零拷贝切片）
pub struct VrmlHeader {
    flavor: VrmlFlavor,
    version: []const u8,
    encoding: []const u8,
    body_offset: usize,
}

/// 单个 VRML 词法单元（零拷贝切片指向原始文本）
pub struct VrmlToken {
    kind: VrmlTokenKind,
    text: []const u8,
    start: usize,
    end: usize,
}

/// VRML 节点声明（支持 DEF / USE / 普通节点，body_slice 零拷贝指向 `{ ... }` 内部）
pub struct VrmlNode {
    def_name: []const u8,
    node_type: []const u8,
    is_use: bool,
    body_slice: []const u8,
}

/// VRML 场景单遍统计汇总
pub struct VrmlStats {
    node_count: u32,
    def_count: u32,
    use_count: u32,
    route_count: u32,
    max_depth: u32,
}

// ---------------------------------------------------------------------------
// 内部零分配辅助函数
// ---------------------------------------------------------------------------

fn isVrmlSpace(c: u8): bool {
    // VRML97 规范明确将逗号 ',' 视为空白分隔符
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
}

fn isLineSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isDelim(c: u8): bool {
    return isVrmlSpace(c) || c == '#' || c == '{' || c == '}' || c == '[' || c == ']' || c == '"';
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

fn isNumberStart(src: []const u8, pos: usize): bool {
    if (pos >= src.len) {
        return false;
    }
    const c = src[pos];
    if (isDigit(c)) {
        return true;
    }
    if (c == '+' || c == '-') {
        if (pos + 1 < src.len) {
            const n = src[pos + 1];
            if (isDigit(n)) return true;
            if (n == '.' && pos + 2 < src.len && isDigit(src[pos + 2])) return true;
        }
        return false;
    }
    if (c == '.') {
        return pos + 1 < src.len && isDigit(src[pos + 1]);
    }
    return false;
}

// ---------------------------------------------------------------------------
// 1. 文件头解析 (#VRML V2.0 utf8 / #X3D V3.x utf8)
// ---------------------------------------------------------------------------

/// 解析首行 `#VRML V2.0 utf8` 或 `#X3D V3.x/4.0 utf8` 头，返回方言、版本、编码及正文偏移
pub fn parseVrmlHeader(data: []const u8): VrmlHeader? {
    if (data.len < 6 || data[0] != '#') {
        return null;
    }
    let line_end: usize = 0;
    while (line_end < data.len && data[line_end] != '\n') {
        line_end += 1;
    }
    const body_offset = if (line_end < data.len && data[line_end] == '\n') (line_end + 1) else line_end;
    let header_end = line_end;
    if (header_end > 0 && data[header_end - 1] == '\r') {
        header_end -= 1;
    }
    const line = data[0..header_end];

    // 第 1 段：#VRML 或 #X3D
    let i: usize = 0;
    while (i < line.len && !isLineSpace(line[i])) {
        i += 1;
    }
    const magic = line[0..i];
    let flavor = VrmlFlavor.unknown;
    if (eqlBytes(magic, "#VRML")) {
        flavor = VrmlFlavor.vrml97;
    } else if (eqlBytes(magic, "#X3D")) {
        flavor = VrmlFlavor.x3d_classic;
    } else {
        return null;
    }

    // 第 2 段：版本号（如 V2.0 / V3.3 / V4.0）
    while (i < line.len && isLineSpace(line[i])) {
        i += 1;
    }
    const ver_start = i;
    while (i < line.len && !isLineSpace(line[i])) {
        i += 1;
    }
    const version = line[ver_start..i];
    if (version.len < 2 || version[0] != 'V') {
        return null;
    }

    // 第 3 段：编码（如 utf8）
    while (i < line.len && isLineSpace(line[i])) {
        i += 1;
    }
    const enc_start = i;
    while (i < line.len && !isLineSpace(line[i])) {
        i += 1;
    }
    const encoding = line[enc_start..i];
    if (encoding.len == 0) {
        return null;
    }

    return VrmlHeader {
        flavor: flavor,
        version: version,
        encoding: encoding,
        body_offset: body_offset,
    };
}

/// 快速检测数据是否以合法 `#VRML` 或 `#X3D` 头开头
pub fn isVrml(data: []const u8): bool {
    return parseVrmlHeader(data) != null;
}

// ---------------------------------------------------------------------------
// 2. 零分配词法分析器 (VrmlLexer)
// ---------------------------------------------------------------------------

/// 零分配 VRML97 / Classic X3D 词法流
/// 自动跳过空白、逗号 `,` 与 `# ... \n` 注释；支持转义双引号字符串、数值（含十六进制与科学计数法）与括号
pub struct VrmlLexer {
    src: []const u8,
    pos: usize,
    malformed: bool,

    pub static fn init(src: []const u8): VrmlLexer {
        return VrmlLexer {
            src: src,
            pos: 0,
            malformed: false,
        };
    }

    pub static fn initAt(src: []const u8, start_pos: usize): VrmlLexer {
        const clamped = if (start_pos <= src.len) start_pos else src.len;
        return VrmlLexer {
            src: src,
            pos: clamped,
            malformed: false,
        };
    }

    fn skipTrivia(*_) {
        const s = _.src;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (isVrmlSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                while (_.pos < s.len && s[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            break;
        }
    }

    pub fn peek(): VrmlToken? {
        let clone = VrmlLexer {
            src: _.src,
            pos: _.pos,
            malformed: _.malformed,
        };
        return clone.next();
    }

    pub fn next(*_): VrmlToken? {
        _.skipTrivia();
        const s = _.src;
        if (_.pos >= s.len) {
            return null;
        }
        const start = _.pos;
        const c = s[start];

        if (c == '{') {
            _.pos += 1;
            return VrmlToken { kind: VrmlTokenKind.lbrace, text: s[start.._.pos], start: start, end: _.pos };
        }
        if (c == '}') {
            _.pos += 1;
            return VrmlToken { kind: VrmlTokenKind.rbrace, text: s[start.._.pos], start: start, end: _.pos };
        }
        if (c == '[') {
            _.pos += 1;
            return VrmlToken { kind: VrmlTokenKind.lbracket, text: s[start.._.pos], start: start, end: _.pos };
        }
        if (c == ']') {
            _.pos += 1;
            return VrmlToken { kind: VrmlTokenKind.rbracket, text: s[start.._.pos], start: start, end: _.pos };
        }
        if (c == '"') {
            _.pos += 1;
            const inner_start = _.pos;
            while (_.pos < s.len) {
                const ch = s[_.pos];
                if (ch == '\\') {
                    if (_.pos + 1 < s.len) {
                        _.pos += 2;
                    } else {
                        _.malformed = true;
                        _.pos = s.len;
                        return null;
                    }
                    continue;
                }
                if (ch == '"') {
                    const inner_end = _.pos;
                    _.pos += 1;
                    return VrmlToken {
                        kind: VrmlTokenKind.string,
                        text: s[inner_start..inner_end],
                        start: start,
                        end: _.pos,
                    };
                }
                _.pos += 1;
            }
            // 未闭合字符串
            _.malformed = true;
            return null;
        }
        if (isNumberStart(s, start)) {
            while (_.pos < s.len && !isDelim(s[_.pos])) {
                _.pos += 1;
            }
            return VrmlToken {
                kind: VrmlTokenKind.number,
                text: s[start.._.pos],
                start: start,
                end: _.pos,
            };
        }
        while (_.pos < s.len && !isDelim(s[_.pos])) {
            _.pos += 1;
        }
        return VrmlToken {
            kind: VrmlTokenKind.ident,
            text: s[start.._.pos],
            start: start,
            end: _.pos,
        };
    }
}

// ---------------------------------------------------------------------------
// 3. 括号块匹配与节点迭代器 (VrmlNodeIter) + 字段定位 (findVrmlField)
// ---------------------------------------------------------------------------

struct BlockSpan {
    inner_start: usize,
    inner_end: usize,
    after_end: usize,
}

/// 当 lex 刚刚消费完 `{`（open_end 为 `{` 后位置）时，匹配到对应的 `}`
fn consumeBraceBlock(lex: *VrmlLexer, open_end: usize): BlockSpan? {
    let depth: u32 = 1;
    while (tok := lex.next()) {
        if (tok.kind == VrmlTokenKind.lbrace) {
            depth += 1;
        } else if (tok.kind == VrmlTokenKind.rbrace) {
            depth -= 1;
            if (depth == 0) {
                return BlockSpan {
                    inner_start: open_end,
                    inner_end: tok.start,
                    after_end: tok.end,
                };
            }
        }
    }
    return null;
}

/// 当 lex 刚刚消费完 `[`（open_end 为 `[` 后位置）时，匹配到对应的 `]`
fn consumeBracketBlock(lex: *VrmlLexer, open_end: usize): BlockSpan? {
    let depth: u32 = 1;
    while (tok := lex.next()) {
        if (tok.kind == VrmlTokenKind.lbracket) {
            depth += 1;
        } else if (tok.kind == VrmlTokenKind.rbracket) {
            depth -= 1;
            if (depth == 0) {
                return BlockSpan {
                    inner_start: open_end,
                    inner_end: tok.start,
                    after_end: tok.end,
                };
            }
        }
    }
    return null;
}

/// 零分配 VRML 节点迭代器（兼容 DEF / USE / ROUTE 与普通节点；既可传入整份文档也可传入 body_slice）
pub struct VrmlNodeIter {
    lex: VrmlLexer,

    pub static fn init(data: []const u8): VrmlNodeIter {
        let start_pos: usize = 0;
        if (hdr := parseVrmlHeader(data)) {
            start_pos = hdr.body_offset;
        }
        return VrmlNodeIter {
            lex: VrmlLexer.initAt(data, start_pos),
        };
    }

    pub fn next(*_): VrmlNode? {
        while (tok := _.lex.next()) {
            if (tok.kind != VrmlTokenKind.ident) {
                continue;
            }
            // 跳过 Classic X3D 头语句
            if (eqlBytes(tok.text, "PROFILE") || eqlBytes(tok.text, "COMPONENT")) {
                * = _.lex.next();
                continue;
            }
            if (eqlBytes(tok.text, "META")) {
                * = _.lex.next();
                * = _.lex.next();
                continue;
            }
            if (eqlBytes(tok.text, "ROUTE")) {
                * = _.lex.next(); // from
                * = _.lex.next(); // TO
                * = _.lex.next(); // to
                continue;
            }
            if (eqlBytes(tok.text, "USE")) {
                const name_tok = _.lex.next() ?? return null;
                if (name_tok.kind != VrmlTokenKind.ident) {
                    return null;
                }
                return VrmlNode {
                    def_name: name_tok.text,
                    node_type: "",
                    is_use: true,
                    body_slice: "",
                };
            }
            if (eqlBytes(tok.text, "DEF")) {
                const name_tok = _.lex.next() ?? return null;
                const type_tok = _.lex.next() ?? return null;
                const lb = _.lex.next() ?? return null;
                if (name_tok.kind != VrmlTokenKind.ident || type_tok.kind != VrmlTokenKind.ident || lb.kind != VrmlTokenKind.lbrace) {
                    return null;
                }
                const span = consumeBraceBlock(&_.lex, lb.end) ?? return null;
                return VrmlNode {
                    def_name: name_tok.text,
                    node_type: type_tok.text,
                    is_use: false,
                    body_slice: _.lex.src[span.inner_start..span.inner_end],
                };
            }

            // 普通节点声明：<Type> { ... }
            if (nxt := _.lex.peek()) {
                if (nxt.kind == VrmlTokenKind.lbrace) {
                    const lb = _.lex.next() ?? return null;
                    const span = consumeBraceBlock(&_.lex, lb.end) ?? return null;
                    return VrmlNode {
                        def_name: "",
                        node_type: tok.text,
                        is_use: false,
                        body_slice: _.lex.src[span.inner_start..span.inner_end],
                    };
                }
            }
        }
        return null;
    }
}

/// 在节点的 `body_slice` 顶层零分配定位指定字段名，返回其字段值切片（如 `1 2 3`、`[ ... ]`、`Box { ... }`）
pub fn findVrmlField(body_slice: []const u8, field_name: []const u8): ([]const u8)? {
    let lex = VrmlLexer.init(body_slice);
    while (tok := lex.next()) {
        if (tok.kind != VrmlTokenKind.ident) {
            continue;
        }
        if (eqlBytes(tok.text, "ROUTE")) {
            * = lex.next();
            * = lex.next();
            * = lex.next();
            continue;
        }
        const cur_field = tok.text;
        const val_first = lex.next() ?? return null;
        const val_start = val_first.start;
        let val_end = val_first.end;

        if (val_first.kind == VrmlTokenKind.lbracket) {
            const span = consumeBracketBlock(&lex, val_first.end) ?? return null;
            val_end = span.after_end;
        } else if (val_first.kind == VrmlTokenKind.lbrace) {
            const span = consumeBraceBlock(&lex, val_first.end) ?? return null;
            val_end = span.after_end;
        } else if (val_first.kind == VrmlTokenKind.ident && eqlBytes(val_first.text, "DEF")) {
            * = lex.next(); // def name
            * = lex.next(); // node type
            const lb = lex.next() ?? return null;
            if (lb.kind != VrmlTokenKind.lbrace) {
                return null;
            }
            const span = consumeBraceBlock(&lex, lb.end) ?? return null;
            val_end = span.after_end;
        } else if (val_first.kind == VrmlTokenKind.ident && eqlBytes(val_first.text, "USE")) {
            const use_name = lex.next() ?? return null;
            val_end = use_name.end;
        } else if (val_first.kind == VrmlTokenKind.ident) {
            // 可能是 SFNode: `Type { ... }`，否则是单值如 `TRUE`/`FALSE`/`NULL`
            if (nxt := lex.peek()) {
                if (nxt.kind == VrmlTokenKind.lbrace) {
                    const lb = lex.next() ?? return null;
                    const span = consumeBraceBlock(&lex, lb.end) ?? return null;
                    val_end = span.after_end;
                }
            }
        } else if (val_first.kind == VrmlTokenKind.number) {
            // 连续数字向量字段（如 SFVec3f translation 1 2 3 / SFRotation 0 1 0 1.57）
            while (nxt := lex.peek()) {
                if (nxt.kind != VrmlTokenKind.number) {
                    break;
                }
                const ntok = lex.next() ?? break;
                val_end = ntok.end;
            }
        }

        if (eqlBytes(cur_field, field_name)) {
            return body_slice[val_start..val_end];
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// 4. 单遍场景统计与括号平衡校验 (summarizeVrml)
// ---------------------------------------------------------------------------

/// 单遍扫描整份 VRML/Classic X3D 文档，统计节点数、DEF/USE/ROUTE 数量与最大嵌套深度，
/// 同时严格校验 `{}`/`[]` 括号平衡与字符串闭合；非法文档返回 `null`
pub fn summarizeVrml(data: []const u8): VrmlStats? {
    const hdr = parseVrmlHeader(data) ?? return null;
    let lex = VrmlLexer.initAt(data, hdr.body_offset);

    let stats = VrmlStats {
        node_count: 0,
        def_count: 0,
        use_count: 0,
        route_count: 0,
        max_depth: 0,
    };

    let stack: [64]u8 = [0; 64];
    let sp: usize = 0;
    let brace_depth: u32 = 0;
    let prev_was_ident = false;

    while (tok := lex.next()) {
        if (tok.kind == VrmlTokenKind.lbrace) {
            if (sp >= 64) {
                return null;
            }
            stack[sp] = '{';
            sp += 1;
            brace_depth += 1;
            if (brace_depth > stats.max_depth) {
                stats.max_depth = brace_depth;
            }
            if (prev_was_ident) {
                stats.node_count += 1;
            }
            prev_was_ident = false;
        } else if (tok.kind == VrmlTokenKind.rbrace) {
            if (sp == 0 || stack[sp - 1] != '{') {
                return null;
            }
            sp -= 1;
            brace_depth -= 1;
            prev_was_ident = false;
        } else if (tok.kind == VrmlTokenKind.lbracket) {
            if (sp >= 64) {
                return null;
            }
            stack[sp] = '[';
            sp += 1;
            prev_was_ident = false;
        } else if (tok.kind == VrmlTokenKind.rbracket) {
            if (sp == 0 || stack[sp - 1] != '[') {
                return null;
            }
            sp -= 1;
            prev_was_ident = false;
        } else if (tok.kind == VrmlTokenKind.ident) {
            if (eqlBytes(tok.text, "DEF")) {
                stats.def_count += 1;
            } else if (eqlBytes(tok.text, "USE")) {
                stats.use_count += 1;
            } else if (eqlBytes(tok.text, "ROUTE")) {
                stats.route_count += 1;
            }
            prev_was_ident = true;
        } else {
            prev_was_ident = false;
        }
    }

    if (lex.malformed || sp != 0) {
        return null;
    }
    return stats;
}
