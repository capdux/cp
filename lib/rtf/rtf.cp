// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RTF 1.9 Lexer & Text Extractor)

/// RTF 1.9 魔数与最大嵌套组深度常量
pub const RTF_MAGIC: []const u8 = "{\\rtf";
pub const MAX_RTF_DEPTH: usize = 64;

/// RTF 词法 Token 类型枚举
pub enum RtfTokenKind: u8 {
    group_start = 0,
    group_end = 1,
    control_word = 2,
    control_symbol = 3,
    hex_byte = 4,
    text = 5,
}

/// RTF 文档字符集枚举
pub enum RtfCharset: u8 {
    unknown = 0,
    ansi = 1,
    mac = 2,
    pc = 3,
    pca = 4,
}

/// RTF 词法 Token 零拷贝视图
pub struct RtfToken {
    kind: RtfTokenKind,
    name: []const u8,
    param: i32,
    has_param: bool,
    byte_val: u8,
}

/// RTF 文档头元信息
pub struct RtfHeaderInfo {
    version: u32,
    charset: RtfCharset,
    ansicpg: u32,
    default_font: i32,
    uc_skip: u32,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
    return null;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn isSkipDestination(word: []const u8): bool {
    return strEq(word, "fonttbl")
        || strEq(word, "colortbl")
        || strEq(word, "stylesheet")
        || strEq(word, "info")
        || strEq(word, "pict")
        || strEq(word, "object")
        || strEq(word, "header")
        || strEq(word, "footer")
        || strEq(word, "listtable")
        || strEq(word, "revtbl");
}

/// 跳过前导空白后校验数据是否以 `"{\\rtf"` 开头
pub fn isRtf(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len && isSpace(data[i])) {
        i += 1;
    }
    if (data.len - i < RTF_MAGIC.len) return false;
    return strEq(data[i .. i + RTF_MAGIC.len], RTF_MAGIC);
}

/// 零堆分配 RTF 1.9 词法分析器
pub struct RtfLexer {
    data: []const u8,
    pos: usize = 0,
    had_error: bool = false,

    static fn init(data: []const u8): RtfLexer {
        return RtfLexer {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): RtfToken? {
        if (_.had_error) return null;

        // 跳过裸 CR / LF（RTF 规范中裸换行符无语义）
        while (_.pos < _.data.len && (_.data[_.pos] == '\r' || _.data[_.pos] == '\n')) {
            _.pos += 1;
        }
        if (_.pos >= _.data.len) return null;

        const c = _.data[_.pos];
        if (c == '{') {
            _.pos += 1;
            return RtfToken { kind: RtfTokenKind.group_start, name: "{", param: 0, has_param: false, byte_val: '{' };
        }
        if (c == '}') {
            _.pos += 1;
            return RtfToken { kind: RtfTokenKind.group_end, name: "}", param: 0, has_param: false, byte_val: '}' };
        }

        if (c == '\\') {
            _.pos += 1;
            if (_.pos >= _.data.len) {
                _.had_error = true;
                return null;
            }
            const nc = _.data[_.pos];

            // Hex escape \'hh
            if (nc == '\'') {
                if (_.data.len - _.pos < 3) {
                    _.had_error = true;
                    return null;
                }
                let hi: u8 = 0;
                let lo: u8 = 0;
                if (h := hexVal(_.data[_.pos + 1])) {
                    hi = h;
                } else {
                    _.had_error = true;
                    return null;
                }
                if (l := hexVal(_.data[_.pos + 2])) {
                    lo = l;
                } else {
                    _.had_error = true;
                    return null;
                }
                _.pos += 3;
                return RtfToken {
                    kind: RtfTokenKind.hex_byte,
                    name: "",
                    param: 0,
                    has_param: false,
                    byte_val: (hi << 4) | lo,
                };
            }

            // Control word: \<letters>[<optional -digits>][<optional single space>]
            if (isAlpha(nc)) {
                const wstart = _.pos;
                while (_.pos < _.data.len && isAlpha(_.data[_.pos])) {
                    _.pos += 1;
                }
                const word = _.data[wstart.._.pos];

                let has_param = false;
                let param: i32 = 0;
                if (_.pos < _.data.len && (_.data[_.pos] == '-' || isDigit(_.data[_.pos]))) {
                    let neg = false;
                    let p_idx = _.pos;
                    if (_.data[p_idx] == '-') {
                        neg = true;
                        p_idx += 1;
                    }
                    if (p_idx < _.data.len && isDigit(_.data[p_idx])) {
                        has_param = true;
                        let acc: i64 = 0;
                        while (p_idx < _.data.len && isDigit(_.data[p_idx])) {
                            const d: i64 = (_.data[p_idx] - '0') as i64;
                            if (acc < 2147483648) {
                                acc = acc * 10 + d;
                            }
                            p_idx += 1;
                        }
                        if (neg) {
                            if (acc >= 2147483648) {
                                param = -2147483647 - 1;
                            } else {
                                param = -(acc as i32);
                            }
                        } else {
                            if (acc > 2147483647) {
                                param = 2147483647;
                            } else {
                                param = acc as i32;
                            }
                        }
                        _.pos = p_idx;
                    }
                }

                // Consume single trailing space delimiter per RTF 1.9 spec
                if (_.pos < _.data.len && _.data[_.pos] == ' ') {
                    _.pos += 1;
                }

                return RtfToken {
                    kind: RtfTokenKind.control_word,
                    name: word,
                    param: param,
                    has_param: has_param,
                    byte_val: 0,
                };
            }

            // Control symbol: \<non-alpha>
            _.pos += 1;
            return RtfToken {
                kind: RtfTokenKind.control_symbol,
                name: "",
                param: 0,
                has_param: false,
                byte_val: nc,
            };
        }

        // Plain text run
        const tstart = _.pos;
        while (_.pos < _.data.len) {
            const ch = _.data[_.pos];
            if (ch == '{' || ch == '}' || ch == '\\' || ch == '\r' || ch == '\n') break;
            _.pos += 1;
        }
        return RtfToken {
            kind: RtfTokenKind.text,
            name: _.data[tstart.._.pos],
            param: 0,
            has_param: false,
            byte_val: 0,
        };
    }
}

/// 扫描 RTF 头部提取版本、字符集、`ansicpg`、`deff` 与 `uc` 参数
pub fn parseRtfHeader(data: []const u8): RtfHeaderInfo? {
    if (!isRtf(data)) return null;

    let start: usize = 0;
    while (start < data.len && isSpace(data[start])) {
        start += 1;
    }

    let lex = RtfLexer.init(data[start..data.len]);
    if (t0 := lex.next()) {
        if (t0.kind != RtfTokenKind.group_start) return null;
    } else {
        return null;
    }

    let info = RtfHeaderInfo {
        version: 1,
        charset: RtfCharset.unknown,
        ansicpg: 0,
        default_font: 0,
        uc_skip: 1,
    };

    if (t1 := lex.next()) {
        if (t1.kind != RtfTokenKind.control_word || !strEq(t1.name, "rtf")) return null;
        if (t1.has_param && t1.param >= 0) {
            info.version = t1.param as u32;
        }
    } else {
        return null;
    }

    let depth: usize = 1;
    while (tok := lex.next()) {
        if (tok.kind == RtfTokenKind.group_start) {
            depth += 1;
            continue;
        }
        if (tok.kind == RtfTokenKind.group_end) {
            if (depth == 0) return null;
            depth -= 1;
            if (depth == 0) break;
            continue;
        }
        if (depth > 1) continue;

        if (tok.kind == RtfTokenKind.control_word) {
            if (strEq(tok.name, "ansi")) {
                info.charset = RtfCharset.ansi;
            } else if (strEq(tok.name, "mac")) {
                info.charset = RtfCharset.mac;
            } else if (strEq(tok.name, "pc")) {
                info.charset = RtfCharset.pc;
            } else if (strEq(tok.name, "pca")) {
                info.charset = RtfCharset.pca;
            } else if (strEq(tok.name, "ansicpg") && tok.has_param && tok.param >= 0) {
                info.ansicpg = tok.param as u32;
            } else if (strEq(tok.name, "deff") && tok.has_param) {
                info.default_font = tok.param;
            } else if (strEq(tok.name, "uc") && tok.has_param && tok.param >= 0) {
                info.uc_skip = tok.param as u32;
            }
        } else if (tok.kind == RtfTokenKind.text) {
            break;
        }
    }

    if (lex.had_error) return null;
    return info;
}

fn emitByte(out: []u8, out_len: *usize, b: u8): bool {
    const p = out_len.*;
    if (p >= out.len) return false;
    out[p] = b;
    out_len.* = p + 1;
    return true;
}

fn emitUtf8(out: []u8, out_len: *usize, cp: u32): bool {
    if (cp <= 0x7F) {
        return emitByte(out, out_len, cp as u8);
    }
    if (cp <= 0x7FF) {
        if (!emitByte(out, out_len, (0xC0 | ((cp >> 6) & 0x1F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    if (cp <= 0xFFFF) {
        if (!emitByte(out, out_len, (0xE0 | ((cp >> 12) & 0x0F)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 6) & 0x3F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    if (cp <= 0x10FFFF) {
        if (!emitByte(out, out_len, (0xF0 | ((cp >> 18) & 0x07)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 12) & 0x3F)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 6) & 0x3F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    return true;
}

/// 零堆分配提取 RTF 纯文本到调用方缓冲区 `out`（跳过非正文目的地组、处理 `\par`/`\tab`/`\'hh`/`\uN` 与 `\ucN` 替换跳过）
pub fn extractRtfText(data: []const u8, out: []u8): usize? {
    if (!isRtf(data)) return null;

    let start: usize = 0;
    while (start < data.len && isSpace(data[start])) {
        start += 1;
    }

    let uc_stack: [64]u32 = undefined;
    let skip_stack: [64]bool = undefined;
    let depth: usize = 0;
    let just_opened = false;
    let skip_chars: u32 = 0;
    let out_len: usize = 0;

    let lex = RtfLexer.init(data[start..data.len]);
    while (tok := lex.next()) {
        if (tok.kind == RtfTokenKind.group_start) {
            if (depth >= MAX_RTF_DEPTH) return null;
            const parent_uc: u32 = if (depth > 0) uc_stack[depth - 1] else 1;
            const parent_skip: bool = if (depth > 0) skip_stack[depth - 1] else false;
            uc_stack[depth] = parent_uc;
            skip_stack[depth] = parent_skip;
            depth += 1;
            just_opened = true;
            continue;
        }

        if (tok.kind == RtfTokenKind.group_end) {
            if (depth == 0) return null;
            depth -= 1;
            just_opened = false;
            if (depth == 0) break;
            continue;
        }

        if (depth == 0) return null;
        const cur_idx = depth - 1;

        if (tok.kind == RtfTokenKind.control_symbol) {
            if (just_opened && tok.byte_val == '*') {
                skip_stack[cur_idx] = true;
                just_opened = false;
                continue;
            }
            just_opened = false;
            if (skip_stack[cur_idx]) continue;

            if (tok.byte_val == '~') {
                if (!emitByte(out, *out_len, ' ')) return null;
            } else if (tok.byte_val == '_') {
                if (!emitByte(out, *out_len, '-')) return null;
            } else if (tok.byte_val == '\\' || tok.byte_val == '{' || tok.byte_val == '}') {
                if (skip_chars > 0) {
                    skip_chars -= 1;
                } else {
                    if (!emitByte(out, *out_len, tok.byte_val)) return null;
                }
            } else if (tok.byte_val == '\r' || tok.byte_val == '\n') {
                if (!emitByte(out, *out_len, '\n')) return null;
            }
            continue;
        }

        if (tok.kind == RtfTokenKind.control_word) {
            if (just_opened && isSkipDestination(tok.name)) {
                skip_stack[cur_idx] = true;
                just_opened = false;
                continue;
            }
            just_opened = false;
            if (skip_stack[cur_idx]) continue;

            if (strEq(tok.name, "par") || strEq(tok.name, "line")) {
                if (!emitByte(out, *out_len, '\n')) return null;
            } else if (strEq(tok.name, "tab")) {
                if (!emitByte(out, *out_len, '\t')) return null;
            } else if (strEq(tok.name, "uc") && tok.has_param && tok.param >= 0) {
                uc_stack[cur_idx] = tok.param as u32;
            } else if (strEq(tok.name, "u") && tok.has_param) {
                const cp: u32 = if (tok.param < 0) ((tok.param + 65536) as u32) else (tok.param as u32);
                if (!emitUtf8(out, *out_len, cp)) return null;
                skip_chars = uc_stack[cur_idx];
            }
            continue;
        }

        just_opened = false;
        if (skip_stack[cur_idx]) continue;

        if (tok.kind == RtfTokenKind.hex_byte) {
            if (skip_chars > 0) {
                skip_chars -= 1;
            } else {
                if (!emitByte(out, *out_len, tok.byte_val)) return null;
            }
            continue;
        }

        if (tok.kind == RtfTokenKind.text) {
            let i: usize = 0;
            while (i < tok.name.len) {
                if (skip_chars > 0) {
                    skip_chars -= 1;
                } else {
                    if (!emitByte(out, *out_len, tok.name[i])) return null;
                }
                i += 1;
            }
        }
    }

    if (lex.had_error || depth != 0) return null;
    return out_len;
}
