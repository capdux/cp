// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Graphviz DOT Graph Parser & Writer)

/// Graphviz DOT 图类型枚举
pub enum DotGraphKind: u8 {
    graph = 0,
    digraph = 1,
}

/// Graphviz DOT 图头部元数据（`[strict] (graph|digraph) [name] {`）
pub struct DotHeader {
    is_strict: bool = false,
    kind: DotGraphKind,
    name: ([]const u8)? = null,
}

/// 零拷贝 Graphviz DOT 边结构
pub struct DotEdge {
    from_node: []const u8,
    to_node: []const u8,
    is_directed: bool,
    attrs: ([]const u8)? = null,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdChar(c: u8): bool {
    return isAlpha(c) || isDigit(c) || c == '.';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn skipTrivia(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (isSpace(s[i])) {
            i += 1;
        } else if (s[i] == '#') {
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
        } else if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
        } else if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) {
                i += 1;
            }
            if (i + 1 < s.len) {
                i += 2;
            } else {
                return s.len;
            }
        } else {
            break;
        }
    }
    return i;
}

struct IdResult {
    text: []const u8,
    next_pos: usize,
}

fn readId(s: []const u8, start: usize): IdResult? {
    if (start >= s.len) return null;
    if (s[start] == '"') {
        let i = start + 1;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (s[i] == '"') {
                return IdResult {
                    text: s[start + 1 .. i],
                    next_pos: i + 1,
                };
            }
            i += 1;
        }
        return null;
    }
    if (!isIdChar(s[start])) return null;
    let i = start;
    while (i < s.len && isIdChar(s[i])) {
        i += 1;
    }
    return IdResult {
        text: s[start..i],
        next_pos: i,
    };
}

struct HeaderScan {
    header: DotHeader,
    body_start: usize,
}

fn scanDotHeader(input: []const u8): HeaderScan? {
    let pos = skipTrivia(input, 0);
    const tok1 = readId(input, pos) ?? return null;
    let is_strict = false;
    let kind_tok = tok1;
    pos = tok1.next_pos;

    if (eqlIgnoreCaseAscii(tok1.text, "strict")) {
        is_strict = true;
        pos = skipTrivia(input, pos);
        kind_tok = readId(input, pos) ?? return null;
        pos = kind_tok.next_pos;
    }

    let kind = DotGraphKind.graph;
    if (eqlIgnoreCaseAscii(kind_tok.text, "digraph")) {
        kind = DotGraphKind.digraph;
    } else if (eqlIgnoreCaseAscii(kind_tok.text, "graph")) {
        kind = DotGraphKind.graph;
    } else {
        return null;
    }

    pos = skipTrivia(input, pos);
    if (pos >= input.len) return null;

    let name: ([]const u8)? = null;
    if (input[pos] != '{') {
        const name_res = readId(input, pos) ?? return null;
        name = name_res.text;
        pos = skipTrivia(input, name_res.next_pos);
        if (pos >= input.len || input[pos] != '{') {
            return null;
        }
    }

    return HeaderScan {
        header: DotHeader {
            is_strict: is_strict,
            kind: kind,
            name: name,
        },
        body_start: pos + 1,
    };
}

/// 解析 Graphviz DOT 图头部 `[strict] (graph|digraph) [name] {`（自动跳过 `//`、`/* */`、`#` 注释）
pub fn parseDotHeader(input: []const u8): DotHeader? {
    if (scanned := scanDotHeader(input)) {
        return scanned.header;
    }
    return null;
}

struct BracketSpan {
    inner: []const u8,
    next_pos: usize,
}

fn readBracketBlock(s: []const u8, open_pos: usize): BracketSpan? {
    if (open_pos >= s.len || s[open_pos] != '[') return null;
    let i = open_pos + 1;
    let depth: usize = 1;
    let in_quotes = false;
    while (i < s.len) {
        const c = s[i];
        if (in_quotes) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == '"') {
                in_quotes = false;
            }
            i += 1;
            continue;
        }
        if (c == '"') {
            in_quotes = true;
        } else if (c == '[') {
            depth += 1;
        } else if (c == ']') {
            depth -= 1;
            if (depth == 0) {
                return BracketSpan {
                    inner: s[open_pos + 1 .. i],
                    next_pos: i + 1,
                };
            }
        }
        i += 1;
    }
    return null;
}

fn isEdgeOpAt(s: []const u8, pos: usize): bool {
    if (pos + 1 >= s.len) return false;
    return s[pos] == '-' && (s[pos + 1] == '>' || s[pos + 1] == '-');
}

/// 零堆分配 Graphviz DOT 边流式迭代器（支持 `a -> b`、`a -- b`、链式边 `a -> b -> c` 与双引号节点名）
pub struct DotEdgeIter {
    input: []const u8,
    pos: usize = 0,
    prev_chain_node: ([]const u8)? = null,

    pub static fn init(input: []const u8): DotEdgeIter {
        let start_pos: usize = 0;
        if (scanned := scanDotHeader(input)) {
            start_pos = scanned.body_start;
        }
        return DotEdgeIter {
            input: input,
            pos: start_pos,
            prev_chain_node: null,
        };
    }

    /// 读取下一条 `DotEdge`；遍历结束返回 `null`
    pub fn next(*_): DotEdge? {
        // Continue edge chain if previous edge's target is followed by -> or --
        if (chain_from := _.prev_chain_node) {
            const p = skipTrivia(_.input, _.pos);
            if (isEdgeOpAt(_.input, p)) {
                const directed = _.input[p + 1] == '>';
                const after_op = skipTrivia(_.input, p + 2);
                if (rhs := readId(_.input, after_op)) {
                    let next_p = skipTrivia(_.input, rhs.next_pos);
                    let attrs: ([]const u8)? = null;
                    if (isEdgeOpAt(_.input, next_p)) {
                        _.prev_chain_node = rhs.text;
                        _.pos = next_p;
                    } else {
                        _.prev_chain_node = null;
                        if (next_p < _.input.len && _.input[next_p] == '[') {
                            if (blk := readBracketBlock(_.input, next_p)) {
                                attrs = blk.inner;
                                next_p = blk.next_pos;
                            }
                        }
                        _.pos = next_p;
                    }
                    return DotEdge {
                        from_node: chain_from,
                        to_node: rhs.text,
                        is_directed: directed,
                        attrs: attrs,
                    };
                }
            }
            _.prev_chain_node = null;
        }

        while (_.pos < _.input.len) {
            _.pos = skipTrivia(_.input, _.pos);
            if (_.pos >= _.input.len) {
                return null;
            }
            const c = _.input[_.pos];
            if (c == ';' || c == ',' || c == '{' || c == '}') {
                _.pos += 1;
                continue;
            }
            if (c == '[') {
                if (blk := readBracketBlock(_.input, _.pos)) {
                    _.pos = blk.next_pos;
                    continue;
                }
                _.pos = _.input.len;
                return null;
            }

            const lhs_opt = readId(_.input, _.pos);
            if (lhs_opt == null) {
                _.pos += 1;
                continue;
            }
            const lhs = lhs_opt.?;
            const after_lhs = skipTrivia(_.input, lhs.next_pos);

            // Skip keyword prefixes like "subgraph"
            if (eqlIgnoreCaseAscii(lhs.text, "subgraph")) {
                _.pos = after_lhs;
                continue;
            }

            // Attribute assignment: key = value
            if (after_lhs < _.input.len && _.input[after_lhs] == '=') {
                const after_eq = skipTrivia(_.input, after_lhs + 1);
                if (rhs_val := readId(_.input, after_eq)) {
                    _.pos = rhs_val.next_pos;
                } else {
                    _.pos = after_eq;
                }
                continue;
            }

            // Edge operator: lhs -> rhs or lhs -- rhs
            if (isEdgeOpAt(_.input, after_lhs)) {
                const directed = _.input[after_lhs + 1] == '>';
                const after_op = skipTrivia(_.input, after_lhs + 2);
                const rhs_opt = readId(_.input, after_op);
                if (rhs_opt == null) {
                    _.pos = after_op;
                    continue;
                }
                const rhs = rhs_opt.?;
                let next_p = skipTrivia(_.input, rhs.next_pos);
                let attrs: ([]const u8)? = null;

                if (isEdgeOpAt(_.input, next_p)) {
                    _.prev_chain_node = rhs.text;
                    _.pos = next_p;
                } else {
                    _.prev_chain_node = null;
                    if (next_p < _.input.len && _.input[next_p] == '[') {
                        if (blk := readBracketBlock(_.input, next_p)) {
                            attrs = blk.inner;
                            next_p = blk.next_pos;
                        }
                    }
                    _.pos = next_p;
                }

                return DotEdge {
                    from_node: lhs.text,
                    to_node: rhs.text,
                    is_directed: directed,
                    attrs: attrs,
                };
            }

            // Standalone node or default attribute statement
            _.pos = after_lhs;
        }
        return null;
    }
}

/// 字符串与注释感知校验 Graphviz DOT 中 `{}` 与 `[]` 的嵌套平衡性
pub fn validateDotBraces(input: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        const c = input[i];
        if (c == '#') {
            while (i < input.len && input[i] != '\n') {
                i += 1;
            }
            continue;
        }
        if (i + 1 < input.len && c == '/' && input[i + 1] == '/') {
            i += 2;
            while (i < input.len && input[i] != '\n') {
                i += 1;
            }
            continue;
        }
        if (i + 1 < input.len && c == '/' && input[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < input.len) {
                if (input[i] == '*' && input[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return false;
            continue;
        }
        if (c == '"') {
            i += 1;
            let closed = false;
            while (i < input.len) {
                if (input[i] == '\\') {
                    if (i + 1 >= input.len) return false;
                    i += 2;
                    continue;
                }
                if (input[i] == '"') {
                    i += 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return false;
            continue;
        }

        if (c == '{' || c == '[') {
            if (depth >= 64) return false;
            stack[depth] = c;
            depth += 1;
        } else if (c == '}') {
            if (depth == 0 || stack[depth - 1] != '{') return false;
            depth -= 1;
        } else if (c == ']') {
            if (depth == 0 || stack[depth - 1] != '[') return false;
            depth -= 1;
        }
        i += 1;
    }
    return depth == 0;
}

fn isSimpleDotId(s: []const u8): bool {
    if (s.len == 0) return false;
    if (!isAlpha(s[0])) return false;
    let i: usize = 1;
    while (i < s.len) {
        if (!isAlpha(s[i]) && !isDigit(s[i])) return false;
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendQuoted(out: []u8, pos: *usize, s: []const u8): bool {
    if (!appendBytes(out, pos, "\"")) return false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            if (pos.* + 2 > out.len) return false;
            out[pos.*] = '\\';
            out[pos.* + 1] = c;
            pos.* += 2;
        } else {
            if (pos.* >= out.len) return false;
            out[pos.*] = c;
            pos.* += 1;
        }
        i += 1;
    }
    return appendBytes(out, pos, "\"");
}

fn appendDotId(out: []u8, pos: *usize, id: []const u8): bool {
    if (isSimpleDotId(id)) {
        return appendBytes(out, pos, id);
    }
    return appendQuoted(out, pos, id);
}

/// 格式化输出单条 Graphviz DOT 边语句（必要时自动加引号并转义 `"`/`\`）；缓冲不足返回 `null`
pub fn formatDotEdge(
    from_node: []const u8,
    to_node: []const u8,
    directed: bool,
    label: ([]const u8)?,
    out: []u8,
): ([]const u8)? {
    let pos: usize = 0;
    if (!appendDotId(out, &pos, from_node)) return null;
    if (directed) {
        if (!appendBytes(out, &pos, " -> ")) return null;
    } else {
        if (!appendBytes(out, &pos, " -- ")) return null;
    }
    if (!appendDotId(out, &pos, to_node)) return null;

    if (lbl := label) {
        if (!appendBytes(out, &pos, " [label=")) return null;
        if (!appendQuoted(out, &pos, lbl)) return null;
        if (!appendBytes(out, &pos, "]")) return null;
    }
    if (!appendBytes(out, &pos, ";")) return null;
    return out[0..pos];
}
