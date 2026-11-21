// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Hjson Parser & JSON Converter)
//! Zero-allocation Hjson (Human JSON) parser, comment stripper, validator, and RFC 8259 JSON converter.

pub enum HjsonTokenKind: u8 {
    lbrace = 0,
    rbrace = 1,
    lbracket = 2,
    rbracket = 3,
    colon = 4,
    comma = 5,
    string = 6,
    quoteless_string = 7,
    number = 8,
    bool_lit = 9,
    null_lit = 10,
    key = 11,
    invalid = 12,
}

pub struct HjsonToken {
    kind: HjsonTokenKind,
    lexeme: []const u8,
}

const MAX_HJSON_DEPTH: usize = 64;

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isPunctuator(c: u8): bool {
    return c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ':';
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

struct OutBuf {
    buf: []u8,
    len: usize,

    fn push(*_, c: u8): bool {
        if (_.len >= _.buf.len) return false;
        _.buf[_.len] = c;
        _.len += 1;
        return true;
    }

    fn pushSlice(*_, s: []const u8): bool {
        if (_.len + s.len > _.buf.len) return false;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            _.buf[_.len + i] = s[i];
        }
        _.len += s.len;
        return true;
    }

    fn pushHexByte(*_, b: u8): bool {
        const hex = "0123456789abcdef";
        if (!_.push(hex[(b >> 4) & 0x0F])) return false;
        return _.push(hex[b & 0x0F]);
    }

    fn pushJsonEscapedChar(*_, c: u8): bool {
        if (c == '"') return _.pushSlice("\\\"");
        if (c == '\\') return _.pushSlice("\\\\");
        if (c == '\n') return _.pushSlice("\\n");
        if (c == '\r') return _.pushSlice("\\r");
        if (c == '\t') return _.pushSlice("\\t");
        if (c == 0x08) return _.pushSlice("\\b");
        if (c == 0x0C) return _.pushSlice("\\f");
        if (c < 0x20) {
            if (!_.pushSlice("\\u00")) return false;
            return _.pushHexByte(c);
        }
        return _.push(c);
    }

    fn pushJsonStringRaw(*_, s: []const u8): bool {
        if (!_.push('"')) return false;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (!_.pushJsonEscapedChar(s[i])) return false;
        }
        return _.push('"');
    }
}

fn isStrictJsonNumber(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    if (s[i] == '-') {
        i += 1;
        if (i >= s.len) return false;
    }
    if (s[i] == '0') {
        i += 1;
        if (i < s.len && isAsciiDigit(s[i])) return false;
    } else if (isAsciiDigit(s[i])) {
        while (i < s.len && isAsciiDigit(s[i])) i += 1;
    } else {
        return false;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        if (i >= s.len || !isAsciiDigit(s[i])) return false;
        while (i < s.len && isAsciiDigit(s[i])) i += 1;
    }
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        if (i < s.len && (s[i] == '+' || s[i] == '-')) i += 1;
        if (i >= s.len || !isAsciiDigit(s[i])) return false;
        while (i < s.len && isAsciiDigit(s[i])) i += 1;
    }
    return i == s.len;
}

struct Parser {
    input: []const u8,
    pos: usize,
    out: OutBuf,
    emit: bool,

    fn init(input: []const u8, out: []u8, emit: bool): Parser {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return Parser{
            input: input,
            pos: start,
            out: OutBuf{ buf: out, len: 0 },
            emit: emit,
        };
    }

    fn writeByte(*_, c: u8): bool {
        if (!_.emit) return true;
        return _.out.push(c);
    }

    fn writeSlice(*_, s: []const u8): bool {
        if (!_.emit) return true;
        return _.out.pushSlice(s);
    }

    fn writeJsonEscaped(*_, c: u8): bool {
        if (!_.emit) return true;
        return _.out.pushJsonEscapedChar(c);
    }

    fn writeJsonStringRaw(*_, s: []const u8): bool {
        if (!_.emit) return true;
        return _.out.pushJsonStringRaw(s);
    }

    /// Skips whitespace and #, //, /* */ comments.
    /// Sets `saw_newline.* = true` if any newline was crossed.
    /// Returns false on unclosed block comment.
    fn skipWsAndComments(*_, saw_newline: *bool): bool {
        const s = _.input;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (c == '\n' || c == '\r') {
                saw_newline.* = true;
                _.pos += 1;
                continue;
            }
            if (isHorizSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.pos += 1;
                while (_.pos < s.len && s[_.pos] != '\n' && s[_.pos] != '\r') {
                    _.pos += 1;
                }
                continue;
            }
            if (c == '/' && _.pos + 1 < s.len) {
                if (s[_.pos + 1] == '/') {
                    _.pos += 2;
                    while (_.pos < s.len && s[_.pos] != '\n' && s[_.pos] != '\r') {
                        _.pos += 1;
                    }
                    continue;
                }
                if (s[_.pos + 1] == '*') {
                    _.pos += 2;
                    let closed = false;
                    while (_.pos + 1 < s.len) {
                        if (s[_.pos] == '\n' || s[_.pos] == '\r') {
                            saw_newline.* = true;
                        }
                        if (s[_.pos] == '*' && s[_.pos + 1] == '/') {
                            _.pos += 2;
                            closed = true;
                            break;
                        }
                        _.pos += 1;
                    }
                    if (!closed) return false;
                    continue;
                }
            }
            break;
        }
        return true;
    }

    fn parseQuotedString(*_, quote: u8): bool {
        const s = _.input;
        _.pos += 1; // skip opening quote
        if (!_.writeByte('"')) return false;

        while (_.pos < s.len) {
            const c = s[_.pos];
            if (c == quote) {
                _.pos += 1;
                return _.writeByte('"');
            }
            if (c == '\n' || c == '\r') return false;
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= s.len) return false;
                const esc = s[_.pos];
                if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                    if (!_.writeByte('\\')) return false;
                    if (!_.writeByte(esc)) return false;
                    _.pos += 1;
                    continue;
                }
                if (esc == '\'') {
                    if (!_.writeByte('\'')) return false;
                    _.pos += 1;
                    continue;
                }
                if (esc == 'u') {
                    if (_.pos + 4 >= s.len) return false;
                    if (!isHexDigit(s[_.pos + 1]) || !isHexDigit(s[_.pos + 2]) || !isHexDigit(s[_.pos + 3]) || !isHexDigit(s[_.pos + 4])) {
                        return false;
                    }
                    if (!_.writeByte('\\')) return false;
                    if (!_.writeSlice(s[_.pos .. _.pos + 5])) return false;
                    _.pos += 5;
                    continue;
                }
                return false;
            }
            if (c == '"') {
                if (!_.writeSlice("\\\"")) return false;
                _.pos += 1;
                continue;
            }
            if (c < 0x20) {
                if (!_.writeJsonEscaped(c)) return false;
                _.pos += 1;
                continue;
            }
            if (!_.writeByte(c)) return false;
            _.pos += 1;
        }
        return false;
    }

    fn parseMultilineString(*_): bool {
        const s = _.input;
        // Compute indentation column of opening '''
        let indent: usize = 0;
        let scan = _.pos;
        while (scan > 0 && s[scan - 1] != '\n' && s[scan - 1] != '\r') {
            scan -= 1;
            indent += 1;
        }

        // Consume opening '''
        _.pos += 3;

        // Skip horizontal whitespace and first newline immediately after opening '''
        let ws_end = _.pos;
        while (ws_end < s.len && isHorizSpace(s[ws_end])) ws_end += 1;
        if (ws_end < s.len && s[ws_end] == '\r') {
            ws_end += 1;
            if (ws_end < s.len && s[ws_end] == '\n') ws_end += 1;
            _.pos = ws_end;
        } else if (ws_end < s.len && s[ws_end] == '\n') {
            _.pos = ws_end + 1;
        }

        // Find closing '''
        const body_start = _.pos;
        let body_end: usize = 0;
        let found = false;
        while (_.pos + 2 < s.len) {
            if (s[_.pos] == '\'' && s[_.pos + 1] == '\'' && s[_.pos + 2] == '\'') {
                body_end = _.pos;
                _.pos += 3;
                found = true;
                break;
            }
            _.pos += 1;
        }
        if (!found) return false;

        // Strip trailing spaces/tabs on closing line and one final newline before '''
        let trimmed_end = body_end;
        while (trimmed_end > body_start && isHorizSpace(s[trimmed_end - 1])) {
            trimmed_end -= 1;
        }
        if (trimmed_end > body_start && s[trimmed_end - 1] == '\n') {
            trimmed_end -= 1;
            if (trimmed_end > body_start && s[trimmed_end - 1] == '\r') {
                trimmed_end -= 1;
            }
        } else if (trimmed_end > body_start && s[trimmed_end - 1] == '\r') {
            trimmed_end -= 1;
        } else {
            // Closing ''' was on same line as content: keep trailing spaces before '''
            trimmed_end = body_end;
        }

        if (!_.writeByte('"')) return false;
        let p = body_start;
        let at_line_start = true;
        while (p < trimmed_end) {
            if (at_line_start) {
                let skipped: usize = 0;
                while (p < trimmed_end && skipped < indent && isHorizSpace(s[p])) {
                    p += 1;
                    skipped += 1;
                }
                at_line_start = false;
                if (p >= trimmed_end) break;
            }
            const c = s[p];
            if (c == '\r') {
                p += 1;
                if (p < trimmed_end && s[p] == '\n') p += 1;
                if (!_.writeSlice("\\n")) return false;
                at_line_start = true;
                continue;
            }
            if (c == '\n') {
                p += 1;
                if (!_.writeSlice("\\n")) return false;
                at_line_start = true;
                continue;
            }
            if (!_.writeJsonEscaped(c)) return false;
            p += 1;
        }
        return _.writeByte('"');
    }

    fn parseKey(*_): bool {
        const s = _.input;
        if (_.pos >= s.len) return false;
        const c = s[_.pos];
        if (c == '"' || c == '\'') {
            return _.parseQuotedString(c);
        }
        if (isPunctuator(c)) return false;

        const start = _.pos;
        while (_.pos < s.len) {
            const ch = s[_.pos];
            if (isWhitespace(ch) || isPunctuator(ch)) break;
            if (ch == '#') break;
            if (ch == '/' && _.pos + 1 < s.len && (s[_.pos + 1] == '/' || s[_.pos + 1] == '*')) break;
            _.pos += 1;
        }
        if (_.pos == start) return false;
        return _.writeJsonStringRaw(s[start.._.pos]);
    }

    fn parsePrimitiveOrQuoteless(*_): bool {
        const s = _.input;
        const start = _.pos;

        // First scan candidate primitive token (stops at whitespace, punctuator, or comment start)
        let prim_end = start;
        while (prim_end < s.len) {
            const ch = s[prim_end];
            if (isWhitespace(ch) || isPunctuator(ch) || ch == '#') break;
            if (ch == '/' && prim_end + 1 < s.len && (s[prim_end + 1] == '/' || s[prim_end + 1] == '*')) break;
            prim_end += 1;
        }
        const cand = s[start..prim_end];

        // Check if rest of line after cand only has horizontal spaces / comments / delimiter (',' '}' ']')
        let check = prim_end;
        while (check < s.len && isHorizSpace(s[check])) check += 1;
        const clean_boundary =
            check >= s.len ||
            s[check] == '\n' ||
            s[check] == '\r' ||
            s[check] == ',' ||
            s[check] == '}' ||
            s[check] == ']' ||
            s[check] == '#' ||
            (s[check] == '/' && check + 1 < s.len && (s[check + 1] == '/' || s[check + 1] == '*'));

        if (clean_boundary && cand.len > 0) {
            if (bytesEql(cand, "true") || bytesEql(cand, "false") || bytesEql(cand, "null") || isStrictJsonNumber(cand)) {
                _.pos = prim_end;
                return _.writeSlice(cand);
            }
        }

        // Otherwise: Hjson quoteless string runs to end of line (trimming trailing whitespace)
        let end = start;
        while (end < s.len && s[end] != '\n' && s[end] != '\r') {
            end += 1;
        }
        _.pos = end;
        let trimmed = end;
        while (trimmed > start && isHorizSpace(s[trimmed - 1])) {
            trimmed -= 1;
        }
        if (trimmed == start) return false;
        return _.writeJsonStringRaw(s[start..trimmed]);
    }

    fn parseValue(*_, depth: usize): bool {
        if (depth > MAX_HJSON_DEPTH) return false;
        let saw_nl = false;
        if (!_.skipWsAndComments(&saw_nl)) return false;
        if (_.pos >= _.input.len) return false;

        const s = _.input;
        const c = s[_.pos];
        if (c == '{') return _.parseObject(depth + 1);
        if (c == '[') return _.parseArray(depth + 1);
        if (c == '\'' && _.pos + 2 < s.len && s[_.pos + 1] == '\'' && s[_.pos + 2] == '\'') {
            return _.parseMultilineString();
        }
        if (c == '"' || c == '\'') return _.parseQuotedString(c);
        if (isPunctuator(c)) return false;
        return _.parsePrimitiveOrQuoteless();
    }

    fn parseObject(*_, depth: usize): bool {
        if (depth > MAX_HJSON_DEPTH) return false;
        _.pos += 1; // skip '{'
        if (!_.writeByte('{')) return false;

        let saw_nl = false;
        if (!_.skipWsAndComments(&saw_nl)) return false;
        if (_.pos >= _.input.len) return false;
        if (_.input[_.pos] == '}') {
            _.pos += 1;
            return _.writeByte('}');
        }

        let first = true;
        while (true) {
            if (!first) {
                if (!_.writeByte(',')) return false;
            }
            first = false;

            if (!_.parseKey()) return false;

            let nl_colon = false;
            if (!_.skipWsAndComments(&nl_colon)) return false;
            if (_.pos >= _.input.len || _.input[_.pos] != ':') return false;
            _.pos += 1;
            if (!_.writeByte(':')) return false;

            if (!_.parseValue(depth)) return false;

            let nl_after = false;
            if (!_.skipWsAndComments(&nl_after)) return false;
            if (_.pos >= _.input.len) return false;

            if (_.input[_.pos] == '}') {
                _.pos += 1;
                return _.writeByte('}');
            }
            if (_.input[_.pos] == ',') {
                _.pos += 1;
                let nl_comma = false;
                if (!_.skipWsAndComments(&nl_comma)) return false;
                if (_.pos >= _.input.len) return false;
                if (_.input[_.pos] == '}') {
                    _.pos += 1;
                    return _.writeByte('}');
                }
                // Reject double comma
                if (_.input[_.pos] == ',') return false;
                continue;
            }
            // Optional comma: newline required between members
            if (!nl_after) return false;
        }
    }

    fn parseArray(*_, depth: usize): bool {
        if (depth > MAX_HJSON_DEPTH) return false;
        _.pos += 1; // skip '['
        if (!_.writeByte('[')) return false;

        let saw_nl = false;
        if (!_.skipWsAndComments(&saw_nl)) return false;
        if (_.pos >= _.input.len) return false;
        if (_.input[_.pos] == ']') {
            _.pos += 1;
            return _.writeByte(']');
        }

        let first = true;
        while (true) {
            if (!first) {
                if (!_.writeByte(',')) return false;
            }
            first = false;

            if (!_.parseValue(depth)) return false;

            let nl_after = false;
            if (!_.skipWsAndComments(&nl_after)) return false;
            if (_.pos >= _.input.len) return false;

            if (_.input[_.pos] == ']') {
                _.pos += 1;
                return _.writeByte(']');
            }
            if (_.input[_.pos] == ',') {
                _.pos += 1;
                let nl_comma = false;
                if (!_.skipWsAndComments(&nl_comma)) return false;
                if (_.pos >= _.input.len) return false;
                if (_.input[_.pos] == ']') {
                    _.pos += 1;
                    return _.writeByte(']');
                }
                if (_.input[_.pos] == ',') return false;
                continue;
            }
            if (!nl_after) return false;
        }
    }
}

pub fn hjsonToJson(input: []const u8, out: []u8): ([]const u8)? {
    let p = Parser.init(input, out, true);
    if (!p.parseValue(0)) return null;
    let tail_nl = false;
    if (!p.skipWsAndComments(&tail_nl)) return null;
    if (p.pos != input.len) return null;
    return out[0..p.out.len];
}

pub fn isValidHjson(input: []const u8): bool {
    let dummy: [1]u8 = [0; 1];
    let p = Parser.init(input, dummy[0..], false);
    if (!p.parseValue(0)) return false;
    let tail_nl = false;
    if (!p.skipWsAndComments(&tail_nl)) return false;
    return p.pos == input.len;
}

pub fn stripHjsonComments(input: []const u8, out: []u8): ([]const u8)? {
    let ob = OutBuf{ buf: out, len: 0 };
    let i: usize = 0;

    while (i < input.len) {
        const c = input[i];

        // Triple-quoted multiline string '''...'''
        if (c == '\'' && i + 2 < input.len && input[i + 1] == '\'' && input[i + 2] == '\'') {
            if (!ob.pushSlice("'''")) return null;
            i += 3;
            let closed = false;
            while (i < input.len) {
                if (input[i] == '\'' && i + 2 < input.len && input[i + 1] == '\'' && input[i + 2] == '\'') {
                    if (!ob.pushSlice("'''")) return null;
                    i += 3;
                    closed = true;
                    break;
                }
                if (!ob.push(input[i])) return null;
                i += 1;
            }
            if (!closed) return null;
            continue;
        }

        // Single or double quoted string
        if (c == '"' || c == '\'') {
            const quote = c;
            if (!ob.push(c)) return null;
            i += 1;
            let closed = false;
            while (i < input.len) {
                const ch = input[i];
                if (!ob.push(ch)) return null;
                i += 1;
                if (ch == '\\') {
                    if (i >= input.len) return null;
                    if (!ob.push(input[i])) return null;
                    i += 1;
                    continue;
                }
                if (ch == quote) {
                    closed = true;
                    break;
                }
            }
            if (!closed) return null;
            continue;
        }

        // '#' line comment
        if (c == '#') {
            i += 1;
            while (i < input.len && input[i] != '\n' && input[i] != '\r') {
                i += 1;
            }
            continue;
        }

        // '//' line comment
        if (c == '/' && i + 1 < input.len && input[i + 1] == '/') {
            i += 2;
            while (i < input.len && input[i] != '\n' && input[i] != '\r') {
                i += 1;
            }
            continue;
        }

        // '/* ... */' block comment (preserve newlines inside)
        if (c == '/' && i + 1 < input.len && input[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < input.len) {
                if (input[i] == '*' && input[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                if (input[i] == '\n') {
                    if (!ob.push('\n')) return null;
                }
                i += 1;
            }
            if (!closed) return null;
            continue;
        }

        if (!ob.push(c)) return null;
        i += 1;
    }

    return out[0..ob.len];
}
