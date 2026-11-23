// Reference-hardened against rust-lang/regex (MIT/Apache-2.0)
// Source: cp/lib/regex/compiler.cp (Linear NFA Regex Bytecode Compiler)
// Pure CP zero-C-dependency regular expression parser, Thompson NFA bytecode compiler,
// and static SIMD literal/first-byte prefilter analyzer.

const std = @import("std");

pub const MAX_INSTRUCTIONS: usize = 16384;
pub const MAX_GROUPS: usize = 32;
pub const MAX_SLOTS: usize = (MAX_GROUPS + 1) * 2;
pub const MAX_REPEAT_COUNT: usize = 1000;
pub const MAX_CLASSES: usize = 1024;
pub const MAX_PARSE_DEPTH: usize = 64;
pub const MAX_AST_NODES: usize = 8192;
pub const MAX_PREFIX_LEN: usize = 32;

pub err RegexError {
    InvalidPattern,
    UnmatchedParen,
    UnclosedClass,
    InvalidQuantifier,
    InvalidEscape,
    MaxComplexityExceeded,
    OutOfMemory,
}

pub enum Opcode {
    Char,
    Any,
    AnyAll,
    Class,
    Split,
    Jmp,
    Save,
    AssertStart,
    AssertEnd,
    AssertStartLine,
    AssertEndLine,
    AssertWordBoundary,
    AssertNonWordBoundary,
    Match,
}

/// 静态预过滤策略（对标 rust-lang/regex prefilter）
pub enum PrefilterKind: u8 {
    None = 0,
    AnchoredStart = 1,
    SingleByte = 2,
    TwoBytes = 3,
    FourBytes = 4,
    LiteralPrefix = 5,
}

/// 编译期提取的 SIMD 预过滤元数据
pub struct Prefilter {
    pub kind: PrefilterKind = PrefilterKind.None,
    pub b0: u8 = 0,
    pub b1: u8 = 0,
    pub b2: u8 = 0,
    pub b3: u8 = 0,
    pub prefix_len: usize = 0,
    pub prefix: [MAX_PREFIX_LEN]u8 = [0; MAX_PREFIX_LEN],
}

pub struct CharClass {
    bits: [4]u64 = [0, 0, 0, 0],

    pub fn setByte(*_, c: u8) {
        const idx = usize(c >> 6);
        const bit: u6 = @truncate(c & 63);
        _.bits[idx] |= (u64(1) << bit);
    }

    pub fn setByteCaseFold(*_, c: u8, case_insensitive: bool) {
        _.setByte(c);
        if (case_insensitive) {
            if (c >= 'a' && c <= 'z') {
                _.setByte(c - 32);
            } else if (c >= 'A' && c <= 'Z') {
                _.setByte(c + 32);
            }
        }
    }

    pub fn setRange(*_, lo: u8, hi: u8) {
        let c: usize = usize(lo);
        const end: usize = usize(hi);
        while (c <= end) : (c += 1) {
            _.setByte(u8(c));
        }
    }

    pub fn foldAsciiCase(*_) {
        let c: usize = usize('a');
        while (c <= usize('z')) : (c += 1) {
            const lo = u8(c);
            const hi = u8(c - 32);
            if (_.contains(lo) || _.contains(hi)) {
                _.setByte(lo);
                _.setByte(hi);
            }
        }
    }

    pub fn merge(*_, other: CharClass) {
        for (i of 0..4) {
            _.bits[i] |= other.bits[i];
        }
    }

    pub fn negate(*_) {
        for (i of 0..4) {
            _.bits[i] = ~_.bits[i];
        }
    }

    pub fn contains(c: u8): bool {
        const idx = usize(c >> 6);
        const bit: u6 = @truncate(c & 63);
        return (_.bits[idx] & (u64(1) << bit)) != 0;
    }

    pub fn popcount(): usize {
        let total: usize = 0;
        for (i of 0..4) {
            total += usize(@popCount(_.bits[i]));
        }
        return total;
    }

    pub static fn digits(): CharClass {
        let cc = CharClass{};
        cc.setRange('0', '9');
        return cc;
    }

    pub static fn word(): CharClass {
        let cc = CharClass{};
        cc.setRange('0', '9');
        cc.setRange('A', 'Z');
        cc.setRange('a', 'z');
        cc.setByte('_');
        return cc;
    }

    pub static fn space(): CharClass {
        let cc = CharClass{};
        cc.setByte(' ');
        cc.setByte('\t');
        cc.setByte('\n');
        cc.setByte('\r');
        cc.setByte(0x0B);
        cc.setByte(0x0C);
        return cc;
    }
}

pub struct Inst {
    op: Opcode,
    ch: u8 = 0,
    class_idx: usize = 0,
    slot: usize = 0,
    out1: usize = 0,
    out2: usize = 0,
}

pub struct Program {
    alloc: std.mem.Allocator,
    insts: []Inst = [],
    classes: []CharClass = [],
    num_groups: usize = 0,
    pub prefilter: Prefilter = Prefilter{},

    pub fn deinit(*_) {
        if (_.insts.len > 0) {
            _.alloc.free(_.insts);
            _.insts = [];
        }
        if (_.classes.len > 0) {
            _.alloc.free(_.classes);
            _.classes = [];
        }
    }
}

struct CompileFlags {
    case_insensitive: bool = false,
    multi_line: bool = false,
    dot_all: bool = false,
    swap_greed: bool = false,
}

enum AstKind {
    Empty,
    Char,
    Any,
    AnyAll,
    Class,
    AssertStart,
    AssertEnd,
    AssertStartLine,
    AssertEndLine,
    AssertWordBoundary,
    AssertNonWordBoundary,
    Concat,
    Alt,
    Repeat,
    Group,
}

struct AstNode {
    kind: AstKind,
    ch: u8 = 0,
    class_idx: usize = 0,
    child: usize = 0,
    children_start: usize = 0,
    children_len: usize = 0,
    min: usize = 0,
    max: usize? = null,
    greedy: bool = true,
    group_id: usize = 0,
}

struct BraceRange {
    min: usize,
    max: usize? = null,
}

struct Patch {
    pc: usize,
    is_out2: bool = false,
}

struct Frag {
    start: usize,
    exits: std.ArrayList(Patch) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.exits.deinit(alloc);
    }
}

struct Parser {
    alloc: std.mem.Allocator,
    pattern: []const u8,
    pos: usize = 0,
    depth: usize = 0,
    num_groups: usize = 0,
    flags: CompileFlags = CompileFlags{},
    nodes: std.ArrayList(AstNode) = .empty,
    child_ids: std.ArrayList(usize) = .empty,
    classes: std.ArrayList(CharClass) = .empty,

    fn deinit(*_) {
        _.nodes.deinit(_.alloc);
        _.child_ids.deinit(_.alloc);
        _.classes.deinit(_.alloc);
    }

    fn addNode(*_, node: AstNode): usize ! RegexError {
        if (_.nodes.items.len >= MAX_AST_NODES) {
            return RegexError.MaxComplexityExceeded;
        }
        const idx = _.nodes.items.len;
        _.nodes.append(_.alloc, node) catch return RegexError.OutOfMemory;
        return idx;
    }

    fn addClass(*_, cc: CharClass): usize ! RegexError {
        if (_.classes.items.len >= MAX_CLASSES) {
            return RegexError.MaxComplexityExceeded;
        }
        const idx = _.classes.items.len;
        _.classes.append(_.alloc, cc) catch return RegexError.OutOfMemory;
        return idx;
    }

    fn addCharNode(*_, c: u8): usize ! RegexError {
        if (_.flags.case_insensitive && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            let cc = CharClass{};
            cc.setByteCaseFold(c, true);
            const idx = _.addClass(cc)?;
            return _.addNode(AstNode{ kind: .Class, class_idx: idx });
        }
        return _.addNode(AstNode{ kind: .Char, ch: c });
    }

    fn parse(*_): usize ! RegexError {
        const root = _.parseAlternation()?;
        if (_.pos < _.pattern.len) {
            if (_.pattern[_.pos] == ')') {
                return RegexError.UnmatchedParen;
            }
            return RegexError.InvalidPattern;
        }
        return root;
    }

    fn parseAlternation(*_): usize ! RegexError {
        let branches: std.ArrayList(usize) = .empty;
        defer branches.deinit(_.alloc);

        const saved_flags = _.flags;
        const first = _.parseConcat()?;
        branches.append(_.alloc, first) catch return RegexError.OutOfMemory;

        while (_.pos < _.pattern.len && _.pattern[_.pos] == '|') {
            _.pos += 1;
            _.flags = saved_flags;
            const next_branch = _.parseConcat()?;
            branches.append(_.alloc, next_branch) catch return RegexError.OutOfMemory;
        }

        if (branches.items.len == 1) {
            return branches.items[0];
        }

        const start = _.child_ids.items.len;
        for (id of branches.items) {
            _.child_ids.append(_.alloc, id) catch return RegexError.OutOfMemory;
        }
        return _.addNode(AstNode{
            kind: .Alt,
            children_start: start,
            children_len: branches.items.len,
        });
    }

    fn parseConcat(*_): usize ! RegexError {
        let items: std.ArrayList(usize) = .empty;
        defer items.deinit(_.alloc);

        while (_.pos < _.pattern.len) {
            const c = _.pattern[_.pos];
            if (c == '|' || c == ')') {
                break;
            }
            const piece = _.parseQuantified()?;
            items.append(_.alloc, piece) catch return RegexError.OutOfMemory;
        }

        if (items.items.len == 0) {
            return _.addNode(AstNode{ kind: .Empty });
        }
        if (items.items.len == 1) {
            return items.items[0];
        }

        const start = _.child_ids.items.len;
        for (id of items.items) {
            _.child_ids.append(_.alloc, id) catch return RegexError.OutOfMemory;
        }
        return _.addNode(AstNode{
            kind: .Concat,
            children_start: start,
            children_len: items.items.len,
        });
    }

    fn parseQuantified(*_): usize ! RegexError {
        let atom = _.parseAtom()?;

        if (_.pos >= _.pattern.len) {
            return atom;
        }

        const c = _.pattern[_.pos];
        if (c == '*' || c == '+' || c == '?' || c == '{') {
            const atom_kind = _.nodes.items[atom].kind;
            if (atom_kind == .AssertStart || atom_kind == .AssertEnd ||
                atom_kind == .AssertStartLine || atom_kind == .AssertEndLine ||
                atom_kind == .AssertWordBoundary || atom_kind == .AssertNonWordBoundary) {
                return RegexError.InvalidQuantifier;
            }

            let min_count: usize = 0;
            let max_count: usize? = null;

            if (c == '*') {
                _.pos += 1;
                min_count = 0;
                max_count = null;
            } else if (c == '+') {
                _.pos += 1;
                min_count = 1;
                max_count = null;
            } else if (c == '?') {
                _.pos += 1;
                min_count = 0;
                max_count = 1;
            } else {
                const parsed = _.parseBraceQuantifier()?;
                min_count = parsed.min;
                max_count = parsed.max;
            }

            let greedy = !_.flags.swap_greed;
            if (_.pos < _.pattern.len && _.pattern[_.pos] == '?') {
                greedy = !greedy;
                _.pos += 1;
            }

            // Reject stacked unparenthesized quantifiers like a** or a+?+
            if (_.pos < _.pattern.len) {
                const next_c = _.pattern[_.pos];
                if (next_c == '*' || next_c == '+' || next_c == '?' || next_c == '{') {
                    return RegexError.InvalidQuantifier;
                }
            }

            return _.addNode(AstNode{
                kind: .Repeat,
                child: atom,
                min: min_count,
                max: max_count,
                greedy: greedy,
            });
        }

        return atom;
    }

    fn parseBraceQuantifier(*_): BraceRange ! RegexError {
        if (_.pos >= _.pattern.len || _.pattern[_.pos] != '{') {
            return RegexError.InvalidQuantifier;
        }
        _.pos += 1;

        const min_val = _.parseUint()?;
        if (min_val > MAX_REPEAT_COUNT) {
            return RegexError.MaxComplexityExceeded;
        }

        if (_.pos >= _.pattern.len) {
            return RegexError.InvalidQuantifier;
        }

        if (_.pattern[_.pos] == '}') {
            _.pos += 1;
            return BraceRange{ min: min_val, max: min_val };
        }

        if (_.pattern[_.pos] != ',') {
            return RegexError.InvalidQuantifier;
        }
        _.pos += 1;

        if (_.pos >= _.pattern.len) {
            return RegexError.InvalidQuantifier;
        }

        if (_.pattern[_.pos] == '}') {
            _.pos += 1;
            return BraceRange{ min: min_val, max: null };
        }

        const max_val = _.parseUint()?;
        if (max_val > MAX_REPEAT_COUNT) {
            return RegexError.MaxComplexityExceeded;
        }
        if (max_val < min_val) {
            return RegexError.InvalidQuantifier;
        }

        if (_.pos >= _.pattern.len || _.pattern[_.pos] != '}') {
            return RegexError.InvalidQuantifier;
        }
        _.pos += 1;

        return BraceRange{ min: min_val, max: max_val };
    }

    fn parseUint(*_): usize ! RegexError {
        if (_.pos >= _.pattern.len) {
            return RegexError.InvalidQuantifier;
        }
        const first = _.pattern[_.pos];
        if (first < '0' || first > '9') {
            return RegexError.InvalidQuantifier;
        }
        let val: usize = 0;
        while (_.pos < _.pattern.len) {
            const c = _.pattern[_.pos];
            if (c < '0' || c > '9') break;
            val = val * 10 + usize(c - '0');
            if (val > MAX_REPEAT_COUNT * 10) {
                return RegexError.MaxComplexityExceeded;
            }
            _.pos += 1;
        }
        return val;
    }

    fn parseAtom(*_): usize ! RegexError {
        if (_.pos >= _.pattern.len) {
            return RegexError.InvalidPattern;
        }

        const c = _.pattern[_.pos];
        if (c == '*' || c == '+' || c == '?' || c == '{') {
            return RegexError.InvalidQuantifier;
        }
        if (c == '}') {
            return RegexError.InvalidPattern;
        }
        if (c == '.') {
            _.pos += 1;
            return _.addNode(AstNode{ kind: if (_.flags.dot_all) .AnyAll else .Any });
        }
        if (c == '^') {
            _.pos += 1;
            return _.addNode(AstNode{ kind: if (_.flags.multi_line) .AssertStartLine else .AssertStart });
        }
        if (c == '$') {
            _.pos += 1;
            return _.addNode(AstNode{ kind: if (_.flags.multi_line) .AssertEndLine else .AssertEnd });
        }
        if (c == '[') {
            return _.parseCharClass();
        }
        if (c == '(') {
            return _.parseGroup();
        }
        if (c == '\\') {
            return _.parseEscapeAtom();
        }

        _.pos += 1;
        return _.addCharNode(c);
    }

    fn parseFlagGroup(*_): usize ! RegexError {
        // Called right after consuming "(?"
        let next_flags = _.flags;
        let setting = true;
        let saw_any = false;

        while (_.pos < _.pattern.len) {
            const fc = _.pattern[_.pos];
            if (fc == ')') {
                if (!saw_any) return RegexError.InvalidPattern;
                _.pos += 1;
                _.flags = next_flags;
                return _.addNode(AstNode{ kind: .Empty });
            }
            if (fc == ':') {
                _.pos += 1;
                const prev_flags = _.flags;
                _.flags = next_flags;
                const inner = _.parseAlternation() catch |e| {
                    _.flags = prev_flags;
                    return e;
                };
                _.flags = prev_flags;
                if (_.pos >= _.pattern.len || _.pattern[_.pos] != ')') {
                    return RegexError.UnmatchedParen;
                }
                _.pos += 1;
                return inner;
            }
            if (fc == '-') {
                if (!setting) return RegexError.InvalidPattern;
                setting = false;
                _.pos += 1;
                continue;
            }
            if (fc == 'i') {
                next_flags.case_insensitive = setting;
                saw_any = true;
                _.pos += 1;
            } else if (fc == 'm') {
                next_flags.multi_line = setting;
                saw_any = true;
                _.pos += 1;
            } else if (fc == 's') {
                next_flags.dot_all = setting;
                saw_any = true;
                _.pos += 1;
            } else if (fc == 'U') {
                next_flags.swap_greed = setting;
                saw_any = true;
                _.pos += 1;
            } else {
                return RegexError.InvalidPattern;
            }
        }
        return RegexError.UnmatchedParen;
    }

    fn parseGroup(*_): usize ! RegexError {
        if (_.depth >= MAX_PARSE_DEPTH) {
            return RegexError.MaxComplexityExceeded;
        }
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // consume '('

        if (_.pos < _.pattern.len && _.pattern[_.pos] == '?') {
            _.pos += 1; // consume '?'
            return _.parseFlagGroup();
        }

        if (_.num_groups >= MAX_GROUPS) {
            return RegexError.MaxComplexityExceeded;
        }
        _.num_groups += 1;
        const group_id = _.num_groups;

        const saved_flags = _.flags;
        const inner = _.parseAlternation() catch |e| {
            _.flags = saved_flags;
            return e;
        };
        _.flags = saved_flags;

        if (_.pos >= _.pattern.len || _.pattern[_.pos] != ')') {
            return RegexError.UnmatchedParen;
        }
        _.pos += 1; // consume ')'

        return _.addNode(AstNode{
            kind: .Group,
            child: inner,
            group_id: group_id,
        });
    }

    fn parseHexByte(*_): u8 ! RegexError {
        if (_.pos + 2 > _.pattern.len) {
            return RegexError.InvalidEscape;
        }
        const hi = hexDigitValue(_.pattern[_.pos]) orelse return RegexError.InvalidEscape;
        const lo = hexDigitValue(_.pattern[_.pos + 1]) orelse return RegexError.InvalidEscape;
        _.pos += 2;
        return (hi << 4) | lo;
    }

    fn parseEscapeAtom(*_): usize ! RegexError {
        _.pos += 1; // consume '\'
        if (_.pos >= _.pattern.len) {
            return RegexError.InvalidEscape;
        }
        const esc = _.pattern[_.pos];
        _.pos += 1;

        if (esc == 'A') {
            return _.addNode(AstNode{ kind: .AssertStart });
        }
        if (esc == 'z') {
            return _.addNode(AstNode{ kind: .AssertEnd });
        }
        if (esc == 'b') {
            return _.addNode(AstNode{ kind: .AssertWordBoundary });
        }
        if (esc == 'B') {
            return _.addNode(AstNode{ kind: .AssertNonWordBoundary });
        }
        if (esc == 'd') {
            const idx = _.addClass(CharClass.digits())?;
            return _.addNode(AstNode{ kind: .Class, class_idx: idx });
        }
        if (esc == 'D') {
            let cc = CharClass.digits();
            cc.negate();
            const idx = _.addClass(cc)?;
            return _.addNode(AstNode{ kind: .Class, class_idx: idx });
        }
        if (esc == 'w') {
            const idx = _.addClass(CharClass.word())?;
            return _.addNode(AstNode{ kind: .Class, class_idx: idx });
        }
        if (esc == 'W') {
            let cc = CharClass.word();
            cc.negate();
            const idx = _.addClass(cc)?;
            return _.addNode(AstNode{ kind: .Class, class_idx: idx });
        }
        if (esc == 's') {
            const idx = _.addClass(CharClass.space())?;
            return _.addNode(AstNode{ kind: .Class, class_idx: idx });
        }
        if (esc == 'S') {
            let cc = CharClass.space();
            cc.negate();
            const idx = _.addClass(cc)?;
            return _.addNode(AstNode{ kind: .Class, class_idx: idx });
        }
        if (esc == 'x') {
            const hb = _.parseHexByte()?;
            return _.addCharNode(hb);
        }

        const lit = decodeLiteralEscape(esc)?;
        return _.addCharNode(lit);
    }

    fn parseCharClass(*_): usize ! RegexError {
        _.pos += 1; // consume '['
        let cc = CharClass{};
        let negated = false;

        if (_.pos < _.pattern.len && _.pattern[_.pos] == '^') {
            negated = true;
            _.pos += 1;
        }

        let first = true;
        let has_prev_char = false;
        let prev_char: u8 = 0;

        while (_.pos < _.pattern.len) {
            const c = _.pattern[_.pos];
            if (c == ']' && !first) {
                _.pos += 1;
                if (has_prev_char) {
                    cc.setByte(prev_char);
                }
                if (_.flags.case_insensitive) {
                    cc.foldAsciiCase();
                }
                if (negated) {
                    cc.negate();
                }
                const idx = _.addClass(cc)?;
                return _.addNode(AstNode{ kind: .Class, class_idx: idx });
            }
            first = false;

            if (c == '-' && has_prev_char && _.pos + 1 < _.pattern.len && _.pattern[_.pos + 1] != ']') {
                _.pos += 1; // consume '-'
                const hi = _.parseClassSingleByte()?;
                if (hi < prev_char) {
                    return RegexError.InvalidPattern;
                }
                cc.setRange(prev_char, hi);
                has_prev_char = false;
                continue;
            }

            if (has_prev_char) {
                cc.setByte(prev_char);
                has_prev_char = false;
            }

            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.pattern.len) {
                    return RegexError.UnclosedClass;
                }
                const esc = _.pattern[_.pos];
                _.pos += 1;
                if (esc == 'd') {
                    cc.merge(CharClass.digits());
                } else if (esc == 'D') {
                    let d = CharClass.digits();
                    d.negate();
                    cc.merge(d);
                } else if (esc == 'w') {
                    cc.merge(CharClass.word());
                } else if (esc == 'W') {
                    let w = CharClass.word();
                    w.negate();
                    cc.merge(w);
                } else if (esc == 's') {
                    cc.merge(CharClass.space());
                } else if (esc == 'S') {
                    let s = CharClass.space();
                    s.negate();
                    cc.merge(s);
                } else if (esc == 'x') {
                    prev_char = _.parseHexByte()?;
                    has_prev_char = true;
                } else {
                    prev_char = decodeLiteralEscape(esc)?;
                    has_prev_char = true;
                }
            } else {
                prev_char = c;
                has_prev_char = true;
                _.pos += 1;
            }
        }

        return RegexError.UnclosedClass;
    }

    fn parseClassSingleByte(*_): u8 ! RegexError {
        if (_.pos >= _.pattern.len) {
            return RegexError.UnclosedClass;
        }
        const c = _.pattern[_.pos];
        _.pos += 1;
        if (c == '\\') {
            if (_.pos >= _.pattern.len) {
                return RegexError.UnclosedClass;
            }
            const esc = _.pattern[_.pos];
            _.pos += 1;
            if (esc == 'd' || esc == 'D' || esc == 'w' || esc == 'W' || esc == 's' || esc == 'S') {
                return RegexError.InvalidPattern;
            }
            if (esc == 'x') {
                return _.parseHexByte();
            }
            return decodeLiteralEscape(esc);
        }
        return c;
    }
}

fn hexDigitValue(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn decodeLiteralEscape(esc: u8): u8 ! RegexError {
    if (esc == 'n') return '\n';
    if (esc == 'r') return '\r';
    if (esc == 't') return '\t';
    if (esc == 'a') return 0x07;
    if (esc == 'f') return 0x0C;
    if (esc == 'v') return 0x0B;
    if (esc == '\\' || esc == '.' || esc == '*' || esc == '+' || esc == '?' ||
        esc == '(' || esc == ')' || esc == '[' || esc == ']' || esc == '{' ||
        esc == '}' || esc == '|' || esc == '^' || esc == '$' || esc == '/' ||
        esc == '-' || esc == '_' || esc == ':' || esc == ';' || esc == ',' ||
        esc == '\'' || esc == '"' || esc == '!' || esc == '@' || esc == '#' ||
        esc == '%' || esc == '&' || esc == '=' || esc == '<' || esc == '>' ||
        esc == '~' || esc == '`') {
        return esc;
    }
    return RegexError.InvalidEscape;
}

// ============================================================================
// Static Prefilter Analysis (Anchored / Literal Prefix / First-Byte Set)
// ============================================================================

struct FirstSet {
    nullable: bool = false,
    any_byte: bool = false,
    anchored_start: bool = false,
    bytes: CharClass = CharClass{},
}

fn analyzeFirstSet(
    nodes: []const AstNode,
    child_ids: []const usize,
    classes: []const CharClass,
    node_idx: usize,
    depth: usize,
): FirstSet {
    if (depth > MAX_PARSE_DEPTH) {
        return FirstSet{ nullable: true, any_byte: true };
    }
    const node = nodes[node_idx];
    switch (node.kind) {
        .Empty | .AssertEnd | .AssertStartLine | .AssertEndLine | .AssertWordBoundary | .AssertNonWordBoundary => {
            return FirstSet{ nullable: true };
        },
        .AssertStart => {
            return FirstSet{ nullable: true, anchored_start: true };
        },
        .Char => {
            let fs = FirstSet{};
            fs.bytes.setByte(node.ch);
            return fs;
        },
        .Any | .AnyAll => {
            return FirstSet{ any_byte: true };
        },
        .Class => {
            let fs = FirstSet{};
            fs.bytes = classes[node.class_idx];
            if (fs.bytes.popcount() > 4) {
                fs.any_byte = true;
            }
            return fs;
        },
        .Group => {
            return analyzeFirstSet(nodes, child_ids, classes, node.child, depth + 1);
        },
        .Repeat => {
            let inner = analyzeFirstSet(nodes, child_ids, classes, node.child, depth + 1);
            if (node.min == 0) {
                inner.nullable = true;
                inner.anchored_start = false;
            }
            return inner;
        },
        .Concat => {
            let acc = FirstSet{ nullable: true };
            let i: usize = 0;
            while (i < node.children_len) : (i += 1) {
                const sub = analyzeFirstSet(nodes, child_ids, classes, child_ids[node.children_start + i], depth + 1);
                if (sub.anchored_start) {
                    acc.anchored_start = true;
                }
                if (sub.any_byte) {
                    acc.any_byte = true;
                } else {
                    acc.bytes.merge(sub.bytes);
                }
                if (!sub.nullable) {
                    acc.nullable = false;
                    break;
                }
            }
            return acc;
        },
        .Alt => {
            let acc = FirstSet{ anchored_start: true };
            let i: usize = 0;
            while (i < node.children_len) : (i += 1) {
                const sub = analyzeFirstSet(nodes, child_ids, classes, child_ids[node.children_start + i], depth + 1);
                if (sub.nullable) acc.nullable = true;
                if (!sub.anchored_start) acc.anchored_start = false;
                if (sub.any_byte) {
                    acc.any_byte = true;
                } else {
                    acc.bytes.merge(sub.bytes);
                }
            }
            return acc;
        },
    }
}

fn collectLiteralPrefix(
    nodes: []const AstNode,
    child_ids: []const usize,
    node_idx: usize,
    out: *[MAX_PREFIX_LEN]u8,
    out_len: *usize,
    depth: usize,
): bool {
    // Returns true if we can continue collecting subsequent concat nodes
    if (depth > MAX_PARSE_DEPTH || out_len.* >= MAX_PREFIX_LEN) return false;
    const node = nodes[node_idx];
    switch (node.kind) {
        .Empty => return true,
        .Char => {
            out.*[out_len.*] = node.ch;
            out_len.* += 1;
            return out_len.* < MAX_PREFIX_LEN;
        },
        .Group => {
            return collectLiteralPrefix(nodes, child_ids, node.child, out, out_len, depth + 1);
        },
        .Concat => {
            let i: usize = 0;
            while (i < node.children_len) : (i += 1) {
                if (!collectLiteralPrefix(nodes, child_ids, child_ids[node.children_start + i], out, out_len, depth + 1)) {
                    return false;
                }
            }
            return true;
        },
        .Repeat => {
            if (node.min == 0) return false;
            const child = nodes[node.child];
            if (child.kind == .Char) {
                let r: usize = 0;
                while (r < node.min && out_len.* < MAX_PREFIX_LEN) : (r += 1) {
                    out.*[out_len.*] = child.ch;
                    out_len.* += 1;
                }
            }
            return false;
        },
        else => return false,
    }
}

fn buildPrefilter(
    nodes: []const AstNode,
    child_ids: []const usize,
    classes: []const CharClass,
    root: usize,
): Prefilter {
    const fs = analyzeFirstSet(nodes, child_ids, classes, root, 0);
    if (fs.anchored_start) {
        return Prefilter{ kind: PrefilterKind.AnchoredStart };
    }
    if (fs.nullable) {
        return Prefilter{ kind: PrefilterKind.None };
    }

    // 1. Check mandatory literal prefix (>= 2 bytes)
    let pbuf: [MAX_PREFIX_LEN]u8 = [0; MAX_PREFIX_LEN];
    let plen: usize = 0;
    * = collectLiteralPrefix(nodes, child_ids, root, *pbuf, *plen, 0);
    if (plen >= 2) {
        return Prefilter{
            kind: PrefilterKind.LiteralPrefix,
            b0: pbuf[0],
            prefix_len: plen,
            prefix: pbuf,
        };
    }

    if (fs.any_byte) {
        return Prefilter{ kind: PrefilterKind.None };
    }

    // 2. Extract 1..4 possible first bytes for SIMD indexOfByte / indexOfAny2 / indexOfAny4
    let found: [4]u8 = [0, 0, 0, 0];
    let cnt: usize = 0;
    let c: usize = 0;
    while (c < 256) : (c += 1) {
        if (fs.bytes.contains(u8(c))) {
            if (cnt >= 4) {
                return Prefilter{ kind: PrefilterKind.None };
            }
            found[cnt] = u8(c);
            cnt += 1;
        }
    }

    if (cnt == 1) {
        return Prefilter{ kind: PrefilterKind.SingleByte, b0: found[0] };
    }
    if (cnt == 2) {
        return Prefilter{ kind: PrefilterKind.TwoBytes, b0: found[0], b1: found[1] };
    }
    if (cnt == 3) {
        return Prefilter{ kind: PrefilterKind.FourBytes, b0: found[0], b1: found[1], b2: found[2], b3: found[2] };
    }
    if (cnt == 4) {
        return Prefilter{ kind: PrefilterKind.FourBytes, b0: found[0], b1: found[1], b2: found[2], b3: found[3] };
    }
    return Prefilter{ kind: PrefilterKind.None };
}

struct Emitter {
    alloc: std.mem.Allocator,
    nodes: []AstNode,
    child_ids: []usize,
    insts: std.ArrayList(Inst) = .empty,

    fn deinit(*_) {
        _.insts.deinit(_.alloc);
    }

    fn emitInst(*_, inst: Inst): usize ! RegexError {
        if (_.insts.items.len >= MAX_INSTRUCTIONS) {
            return RegexError.MaxComplexityExceeded;
        }
        const pc = _.insts.items.len;
        _.insts.append(_.alloc, inst) catch return RegexError.OutOfMemory;
        return pc;
    }

    fn patch(*_, exits: []Patch, target: usize) {
        for (p of exits) {
            if (p.is_out2) {
                _.insts.items[p.pc].out2 = target;
            } else {
                _.insts.items[p.pc].out1 = target;
            }
        }
    }

    fn compileNode(*_, node_idx: usize): Frag ! RegexError {
        const node = _.nodes[node_idx];
        switch (node.kind) {
            .Empty => {
                const pc = _.emitInst(Inst{ op: .Jmp })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .Char => {
                const pc = _.emitInst(Inst{ op: .Char, ch: node.ch })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .Any => {
                const pc = _.emitInst(Inst{ op: .Any })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .AnyAll => {
                const pc = _.emitInst(Inst{ op: .AnyAll })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .Class => {
                const pc = _.emitInst(Inst{ op: .Class, class_idx: node.class_idx })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .AssertStart => {
                const pc = _.emitInst(Inst{ op: .AssertStart })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .AssertEnd => {
                const pc = _.emitInst(Inst{ op: .AssertEnd })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .AssertStartLine => {
                const pc = _.emitInst(Inst{ op: .AssertStartLine })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .AssertEndLine => {
                const pc = _.emitInst(Inst{ op: .AssertEndLine })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .AssertWordBoundary => {
                const pc = _.emitInst(Inst{ op: .AssertWordBoundary })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .AssertNonWordBoundary => {
                const pc = _.emitInst(Inst{ op: .AssertNonWordBoundary })?;
                let f = Frag{ start: pc };
                f.exits.append(_.alloc, Patch{ pc: pc, is_out2: false }) catch return RegexError.OutOfMemory;
                return f;
            },
            .Concat => {
                let acc = _.compileNode(_.child_ids[node.children_start])?;
                errdefer acc.deinit(_.alloc);

                let i: usize = 1;
                while (i < node.children_len) : (i += 1) {
                    let next_frag = _.compileNode(_.child_ids[node.children_start + i])?;
                    _.patch(acc.exits.items, next_frag.start);
                    acc.exits.clearRetainingCapacity();
                    for (p of next_frag.exits.items) {
                        acc.exits.append(_.alloc, p) catch {
                            next_frag.deinit(_.alloc);
                            return RegexError.OutOfMemory;
                        };
                    }
                    next_frag.deinit(_.alloc);
                }
                return acc;
            },
            .Alt => {
                let first = _.compileNode(_.child_ids[node.children_start])?;
                errdefer first.deinit(_.alloc);

                let i: usize = 1;
                while (i < node.children_len) : (i += 1) {
                    let rhs = _.compileNode(_.child_ids[node.children_start + i])?;
                    const split_pc = _.emitInst(Inst{
                        op: .Split,
                        out1: first.start,
                        out2: rhs.start,
                    }) catch {
                        rhs.deinit(_.alloc);
                        return RegexError.OutOfMemory;
                    };
                    first.start = split_pc;
                    for (p of rhs.exits.items) {
                        first.exits.append(_.alloc, p) catch {
                            rhs.deinit(_.alloc);
                            return RegexError.OutOfMemory;
                        };
                    }
                    rhs.deinit(_.alloc);
                }
                return first;
            },
            .Group => {
                const open_slot = node.group_id * 2;
                const close_slot = open_slot + 1;
                const save_open = _.emitInst(Inst{ op: .Save, slot: open_slot })?;
                let inner = _.compileNode(node.child)?;
                errdefer inner.deinit(_.alloc);
                const save_close = _.emitInst(Inst{ op: .Save, slot: close_slot })?;

                _.insts.items[save_open].out1 = inner.start;
                _.patch(inner.exits.items, save_close);
                inner.exits.clearRetainingCapacity();
                inner.exits.append(_.alloc, Patch{ pc: save_close, is_out2: false }) catch return RegexError.OutOfMemory;
                inner.start = save_open;
                return inner;
            },
            .Repeat => {
                return _.compileRepeat(node.child, node.min, node.max, node.greedy);
            },
        }
    }

    fn compileRepeat(*_, child_idx: usize, min_count: usize, max_count: usize?, greedy: bool): Frag ! RegexError {
        let has_acc = false;
        let acc = Frag{ start: 0 };
        errdefer acc.deinit(_.alloc);

        for (i of 0..min_count) {
            * = i;
            let part = _.compileNode(child_idx)?;
            if (!has_acc) {
                acc = part;
                has_acc = true;
            } else {
                _.patch(acc.exits.items, part.start);
                acc.exits.clearRetainingCapacity();
                for (p of part.exits.items) {
                    acc.exits.append(_.alloc, p) catch {
                        part.deinit(_.alloc);
                        return RegexError.OutOfMemory;
                    };
                }
                part.deinit(_.alloc);
            }
        }

        if (max_count == null) {
            let body = _.compileNode(child_idx)?;
            errdefer body.deinit(_.alloc);
            const split_pc = _.emitInst(Inst{ op: .Split })?;
            if (greedy) {
                _.insts.items[split_pc].out1 = body.start;
            } else {
                _.insts.items[split_pc].out2 = body.start;
            }
            _.patch(body.exits.items, split_pc);
            body.deinit(_.alloc);

            const exit_patch = Patch{ pc: split_pc, is_out2: greedy };
            if (!has_acc) {
                acc.start = split_pc;
                acc.exits.append(_.alloc, exit_patch) catch return RegexError.OutOfMemory;
                has_acc = true;
            } else {
                _.patch(acc.exits.items, split_pc);
                acc.exits.clearRetainingCapacity();
                acc.exits.append(_.alloc, exit_patch) catch return RegexError.OutOfMemory;
            }
        } else {
            const rem = max_count.? - min_count;
            for (j of 0..rem) {
                * = j;
                let opt_body = _.compileNode(child_idx)?;
                const split_pc = _.emitInst(Inst{ op: .Split }) catch {
                    opt_body.deinit(_.alloc);
                    return RegexError.OutOfMemory;
                };
                if (greedy) {
                    _.insts.items[split_pc].out1 = opt_body.start;
                } else {
                    _.insts.items[split_pc].out2 = opt_body.start;
                }

                const skip_patch = Patch{ pc: split_pc, is_out2: greedy };
                if (!has_acc) {
                    acc.start = split_pc;
                    acc.exits.append(_.alloc, skip_patch) catch {
                        opt_body.deinit(_.alloc);
                        return RegexError.OutOfMemory;
                    };
                    for (p of opt_body.exits.items) {
                        acc.exits.append(_.alloc, p) catch {
                            opt_body.deinit(_.alloc);
                            return RegexError.OutOfMemory;
                        };
                    }
                    has_acc = true;
                } else {
                    _.patch(acc.exits.items, split_pc);
                    acc.exits.clearRetainingCapacity();
                    acc.exits.append(_.alloc, skip_patch) catch {
                        opt_body.deinit(_.alloc);
                        return RegexError.OutOfMemory;
                    };
                    for (p of opt_body.exits.items) {
                        acc.exits.append(_.alloc, p) catch {
                            opt_body.deinit(_.alloc);
                            return RegexError.OutOfMemory;
                        };
                    }
                }
                opt_body.deinit(_.alloc);
            }
        }

        if (!has_acc) {
            const jmp_pc = _.emitInst(Inst{ op: .Jmp })?;
            acc.start = jmp_pc;
            acc.exits.append(_.alloc, Patch{ pc: jmp_pc, is_out2: false }) catch return RegexError.OutOfMemory;
        }

        return acc;
    }
}

pub static class Compiler {
    pub fn compile(alloc: std.mem.Allocator, pattern: []const u8): Program ! RegexError {
        let parser = Parser{ alloc: alloc, pattern: pattern };
        defer parser.deinit();

        const root = parser.parse()?;
        const prefilter = buildPrefilter(
            parser.nodes.items,
            parser.child_ids.items,
            parser.classes.items,
            root,
        );

        let emitter = Emitter{
            alloc: alloc,
            nodes: parser.nodes.items,
            child_ids: parser.child_ids.items,
        };
        defer emitter.deinit();

        const save_start = emitter.emitInst(Inst{ op: .Save, slot: 0 })?;
        let body = emitter.compileNode(root)?;
        defer body.deinit(alloc);
        const save_end = emitter.emitInst(Inst{ op: .Save, slot: 1 })?;
        const match_pc = emitter.emitInst(Inst{ op: .Match })?;

        emitter.insts.items[save_start].out1 = body.start;
        emitter.patch(body.exits.items, save_end);
        emitter.insts.items[save_end].out1 = match_pc;

        let insts_slice = alloc.alloc(Inst, emitter.insts.items.len) catch return RegexError.OutOfMemory;
        errdefer alloc.free(insts_slice);
        for (i of 0..emitter.insts.items.len) {
            insts_slice[i] = emitter.insts.items[i];
        }

        let classes_slice = alloc.alloc(CharClass, parser.classes.items.len) catch return RegexError.OutOfMemory;
        for (i of 0..parser.classes.items.len) {
            classes_slice[i] = parser.classes.items[i];
        }

        return Program{
            alloc: alloc,
            insts: insts_slice,
            classes: classes_slice,
            num_groups: parser.num_groups,
            prefilter: prefilter,
        };
    }
}
