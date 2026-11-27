// Source: Absorbed from /data/dev/bun/src/css/rules/counter_style.cp | Synced: 2026-11-22T07:45:00Z
// Pure-CP zero-allocation W3C CSS Counter Styles Level 3 (`@counter-style`) rule parser,
// descriptor validator (`system`, `symbols`, `additive-symbols`, `negative`, `prefix`,
// `suffix`, `range`, `pad`, `fallback`, `speak-as`), canonical serializer, and live
// counter/marker formatter supporting cyclic, fixed, symbolic, alphabetic, numeric, and additive algorithms.

/// 单条 `@counter-style` 规则支持的最大符号数
pub const CSS_COUNTER_MAX_SYMBOLS: usize = 32;

/// 单条 `@counter-style` 规则 `range` 描述符支持的最大区间数
pub const CSS_COUNTER_MAX_RANGES: usize = 8;

/// CSS `@counter-style` 解析、校验、序列化与求值错误集
pub err CssCounterStyleError {
    InvalidPrelude,
    UnclosedBlock,
    InvalidSystem,
    InvalidSymbols,
    InvalidAdditiveSymbols,
    InvalidRange,
    InvalidPad,
    InvalidSpeakAs,
    TooManySymbols,
    TooManyRanges,
    BufferTooSmall,
    OutOfRange,
}

type CssCntVec16 = @Vector(16, u8);

fn makeCssCntClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1;
    lut['\t'] = 1;
    lut['\r'] = 1;
    lut['\n'] = 1;
    lut[0x0C] = 1;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_' || i >= 0x80) {
            lut[i] |= 2 | 4;
        } else if ((i >= '0' && i <= '9') || i == '-') {
            lut[i] |= 4;
        }
    }
    return lut;
}

fn makeCssCntLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const CSS_CNT_CLASS_LUT: [u8; 256] = makeCssCntClassLut();
const CSS_CNT_LOWER_LUT: [u8; 256] = makeCssCntLowerLut();

fn isWs(c: u8): bool {
    return (CSS_CNT_CLASS_LUT[@as(usize, c)] & 1) != 0;
}

fn toLowerAscii(c: u8): u8 {
    return CSS_CNT_LOWER_LUT[@as(usize, c)];
}

fn isIdentStart(c: u8): bool {
    return (CSS_CNT_CLASS_LUT[@as(usize, c)] & 2) != 0;
}

fn isIdentChar(c: u8): bool {
    return (CSS_CNT_CLASS_LUT[@as(usize, c)] & 4) != 0;
}

fn isValidCustomIdent(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let idx: usize = 0;
    if (s[0] == '-') {
        if (s.len == 1) {
            return false;
        }
        if (s[1] == '-' || isIdentStart(s[1])) {
            idx = 2;
        } else {
            return false;
        }
    } else if (isIdentStart(s[0])) {
        idx = 1;
    } else {
        return false;
    }
    while (idx < s.len) {
        if (!isIdentChar(s[idx])) {
            return false;
        }
        idx += 1;
    }
    return true;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: CssCntVec16 = a[i .. i + 16][0..16].*;
        const vb: CssCntVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) {
            return false;
        }
    }
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: CssCntVec16 = @splat(65 as u8);
    const hi_Z: CssCntVec16 = @splat(90 as u8);
    const case_bit: CssCntVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: CssCntVec16 = a[i .. i + 16][0..16].*;
        const vb: CssCntVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) {
            return false;
        }
    }
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isForbiddenCounterName(name: []const u8): bool {
    return eqlAsciiIgnoreCase(name, "none") ||
        eqlAsciiIgnoreCase(name, "initial") ||
        eqlAsciiIgnoreCase(name, "inherit") ||
        eqlAsciiIgnoreCase(name, "unset") ||
        eqlAsciiIgnoreCase(name, "revert") ||
        eqlAsciiIgnoreCase(name, "revert-layer");
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn skipWsAndComments(s: []const u8, start: usize): usize {
    let i = start;
    const v_star: CssCntVec16 = @splat('*' as u8);
    while (i < s.len) {
        if (isWs(s[i])) {
            i += 1;
            continue;
        }
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 16 < s.len) {
                const v: CssCntVec16 = s[i .. i + 16][0..16].*;
                if (@reduce(.Or, v == v_star)) break;
                i += 16;
            }
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) {
                i += 1;
            }
            if (i + 1 < s.len) {
                i += 2;
            } else {
                i = s.len;
            }
            continue;
        }
        break;
    }
    return i;
}

fn parseI32(s: []const u8): i32? {
    const t = trimWs(s);
    if (t.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (t[0] == '-') {
        neg = true;
        i = 1;
    } else if (t[0] == '+') {
        i = 1;
    }
    if (i >= t.len) {
        return null;
    }
    let acc: i64 = 0;
    while (i < t.len) {
        const c = t[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + ((c - '0') as i64);
        if (acc > 2147483648) {
            return null;
        }
        i += 1;
    }
    if (neg) {
        const v = -acc;
        if (v < -2147483648) {
            return null;
        }
        return v as i32;
    }
    if (acc > 2147483647) {
        return null;
    }
    return acc as i32;
}

struct TokenSlice {
    value: []const u8,
    was_quoted: bool,
    next_pos: usize,
}

/// 读取一个符号或字符串令牌（支持 `"..."`、`'`...`'` 或非空白 bare token）
fn nextSymbolToken(s: []const u8, start: usize): TokenSlice? {
    const i = skipWsAndComments(s, start);
    if (i >= s.len) {
        return null;
    }
    if (s[i] == '"' || s[i] == '\'') {
        const q = s[i];
        let j = i + 1;
        while (j < s.len) {
            if (s[j] == '\\' && j + 1 < s.len) {
                j += 2;
                continue;
            }
            if (s[j] == q) {
                return TokenSlice{
                    value: s[i + 1..j],
                    was_quoted: true,
                    next_pos: j + 1,
                };
            }
            j += 1;
        }
        return null;
    }
    let j = i;
    while (j < s.len && !isWs(s[j]) && s[j] != ',' && s[j] != ';') {
        if (j + 1 < s.len && s[j] == '/' && s[j + 1] == '*') {
            break;
        }
        j += 1;
    }
    if (j == i) {
        return null;
    }
    return TokenSlice{
        value: s[i..j],
        was_quoted: false,
        next_pos: j,
    };
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (pos + src.len > out.len) {
        return err.BufferTooSmall;
    }
    if (src.len > 0) {
        @memcpy(out[pos .. pos + src.len], src);
    }
    return pos + src.len;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeCssQuoted(out: []u8, start: usize, val: []const u8): usize! {
    let pos = writeByte(out, start, '"')?;
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        if (c == '"' || c == '\\') {
            pos = writeByte(out, pos, '\\')?;
        }
        pos = writeByte(out, pos, c)?;
        i += 1;
    }
    pos = writeByte(out, pos, '"')?;
    return pos;
}

fn writeI32(out: []u8, start: usize, val: i32): usize! {
    let pos = start;
    let v: i64 = val as i64;
    if (v < 0) {
        pos = writeByte(out, pos, '-')?;
        v = -v;
    }
    if (v == 0) {
        return writeByte(out, pos, '0');
    }
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    while (v > 0) {
        tmp[n] = ('0' as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    while (n > 0) {
        n -= 1;
        pos = writeByte(out, pos, tmp[n])?;
    }
    return pos;
}

/// 统计 UTF-8 字符串的码点数量（用于 `pad` 最小宽度计算）
fn countUtf8Codepoints(s: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if ((s[i] & 0xC0) != 0x80) {
            count += 1;
        }
        i += 1;
    }
    return count;
}

/// `@counter-style` 计数算法类别
pub enum CssCounterSystemKind: u8 {
    symbolic = 0,
    cyclic = 1,
    numeric = 2,
    alphabetic = 3,
    additive = 4,
    fixed = 5,
    extends = 6,
}

/// `@counter-style` `system` 描述符结构
pub struct CssCounterSystem {
    kind: CssCounterSystemKind,
    /// `fixed <integer>` 的起始值（默认为 `1`）
    fixed_first: i32,
    /// `extends <counter-style-name>` 引用的父样式名
    extends_name: []const u8,

    /// 默认 `symbolic` 系统
    static fn defaultSymbolic(): CssCounterSystem {
        return CssCounterSystem{
            kind: .symbolic,
            fixed_first: 1,
            extends_name: "",
        };
    }

    /// 构造 `fixed <first>` 系统
    static fn fixedFrom(first: i32): CssCounterSystem {
        return CssCounterSystem{
            kind: .fixed,
            fixed_first: first,
            extends_name: "",
        };
    }

    /// 等价比较
    fn eql(other: CssCounterSystem): bool {
        if (_.kind != other.kind) {
            return false;
        }
        if (_.kind == .fixed) {
            return _.fixed_first == other.fixed_first;
        }
        if (_.kind == .extends) {
            return bytesEql(_.extends_name, other.extends_name);
        }
        return true;
    }
}

/// `additive-symbols` 中的单个 `(权重, 符号)` 元组
pub struct CssAdditiveTuple {
    weight: i32,
    symbol: []const u8,
}

/// `range` 描述符中的单个闭区间（支持 `infinite` 开界）
pub struct CssCounterRange {
    has_min: bool,
    min_val: i32,
    has_max: bool,
    max_val: i32,

    /// 判断给定计数器值是否落在该区间内
    fn contains(value: i32): bool {
        if (_.has_min && value < _.min_val) {
            return false;
        }
        if (_.has_max && value > _.max_val) {
            return false;
        }
        return true;
    }

    /// 等价比较
    fn eql(other: CssCounterRange): bool {
        return _.has_min == other.has_min &&
            _.min_val == other.min_val &&
            _.has_max == other.has_max &&
            _.max_val == other.max_val;
    }
}

/// `speak-as` 语音播报策略枚举
pub enum CssCounterSpeakAs: u8 {
    auto_mode = 0,
    bullets = 1,
    numbers = 2,
    words = 3,
    spell_out = 4,
}

/// W3C CSS Counter Styles Level 3 `@counter-style` 完整规则节点与实时格式化器
pub struct CssCounterStyleRule {
    name: []const u8,
    system: CssCounterSystem,
    symbols: [CSS_COUNTER_MAX_SYMBOLS][]const u8,
    symbol_count: usize,
    additive_symbols: [CSS_COUNTER_MAX_SYMBOLS]CssAdditiveTuple,
    additive_count: usize,
    negative_prefix: []const u8,
    negative_suffix: []const u8,
    has_negative: bool,
    prefix: []const u8,
    has_prefix: bool,
    suffix: []const u8,
    has_suffix: bool,
    ranges: [CSS_COUNTER_MAX_RANGES]CssCounterRange,
    range_count: usize,
    pad_width: usize,
    pad_symbol: []const u8,
    has_pad: bool,
    fallback: []const u8,
    has_fallback: bool,
    speak_as: CssCounterSpeakAs,
    has_speak_as: bool,

    /// 初始化空规则（按 W3C 默认值：`system: symbolic`, `negative: "-"`, `suffix: ". "`, `fallback: decimal`）
    static fn initEmpty(name: []const u8): CssCounterStyleRule {
        return CssCounterStyleRule{
            name: name,
            system: CssCounterSystem.defaultSymbolic(),
            symbols: [""; CSS_COUNTER_MAX_SYMBOLS],
            symbol_count: 0,
            additive_symbols: [CssAdditiveTuple{ weight: 0, symbol: "" }; CSS_COUNTER_MAX_SYMBOLS],
            additive_count: 0,
            negative_prefix: "-",
            negative_suffix: "",
            has_negative: false,
            prefix: "",
            has_prefix: false,
            suffix: ". ",
            has_suffix: false,
            ranges: [CssCounterRange{ has_min: false, min_val: 0, has_max: false, max_val: 0 }; CSS_COUNTER_MAX_RANGES],
            range_count: 0,
            pad_width: 0,
            pad_symbol: "",
            has_pad: false,
            fallback: "decimal",
            has_fallback: false,
            speak_as: .auto_mode,
            has_speak_as: false,
        };
    }

    /// 解析完整的 `@counter-style <name> { ... }` 规则并执行规范描述符校验
    static fn parse(input: []const u8): CssCounterStyleRule! {
        let i = skipWsAndComments(input, 0);
        if (i >= input.len || input[i] != '@') {
            return err.InvalidPrelude;
        }
        i += 1;
        const kw_start = i;
        while (i < input.len && isIdentChar(input[i])) {
            i += 1;
        }
        if (!eqlAsciiIgnoreCase(input[kw_start..i], "counter-style")) {
            return err.InvalidPrelude;
        }

        // 解析 <counter-style-name>
        i = skipWsAndComments(input, i);
        const name_start = i;
        while (i < input.len && isIdentChar(input[i])) {
            i += 1;
        }
        const name = input[name_start..i];
        if (!isValidCustomIdent(name) || isForbiddenCounterName(name)) {
            return err.InvalidPrelude;
        }

        // 查找 '{' 与匹配的 '}'
        i = skipWsAndComments(input, i);
        if (i >= input.len || input[i] != '{') {
            return err.UnclosedBlock;
        }
        const block_start = i + 1;
        let depth: usize = 0;
        let in_quote: u8 = 0;
        let block_end: usize = input.len;
        let found_close = false;
        while (i < input.len) {
            const c = input[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < input.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '/' && i + 1 < input.len && input[i + 1] == '*') {
                i += 2;
                while (i + 1 < input.len && !(input[i] == '*' && input[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < input.len) {
                    i += 2;
                } else {
                    return err.UnclosedBlock;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                i += 1;
                continue;
            }
            if (c == '{') {
                depth += 1;
            } else if (c == '}') {
                depth -= 1;
                if (depth == 0) {
                    block_end = i;
                    found_close = true;
                    break;
                }
            }
            i += 1;
        }
        if (!found_close) {
            return err.UnclosedBlock;
        }
        if (skipWsAndComments(input, block_end + 1) != input.len) {
            return err.InvalidPrelude;
        }

        let rule = CssCounterStyleRule.initEmpty(name);
        const body = input[block_start..block_end];
        let pos: usize = 0;
        while (pos < body.len) {
            pos = skipWsAndComments(body, pos);
            if (pos >= body.len) {
                break;
            }
            if (body[pos] == ';') {
                pos += 1;
                continue;
            }
            // 读取描述符名
            const prop_start = pos;
            while (pos < body.len && body[pos] != ':' && body[pos] != ';') {
                pos += 1;
            }
            if (pos >= body.len || body[pos] != ':') {
                return err.InvalidPrelude;
            }
            const prop_name = trimWs(body[prop_start..pos]);
            pos += 1; // skip ':'

            // 扫描值直到未加引号的 ';' 或块尾
            const val_start = pos;
            let v_quote: u8 = 0;
            while (pos < body.len) {
                const vc = body[pos];
                if (v_quote != 0) {
                    if (vc == '\\' && pos + 1 < body.len) {
                        pos += 2;
                        continue;
                    }
                    if (vc == v_quote) {
                        v_quote = 0;
                    }
                    pos += 1;
                    continue;
                }
                if (vc == '/' && pos + 1 < body.len && body[pos + 1] == '*') {
                    pos += 2;
                    while (pos + 1 < body.len && !(body[pos] == '*' && body[pos + 1] == '/')) {
                        pos += 1;
                    }
                    if (pos + 1 < body.len) {
                        pos += 2;
                    } else {
                        return err.UnclosedBlock;
                    }
                    continue;
                }
                if (vc == '"' || vc == '\'') {
                    v_quote = vc;
                    pos += 1;
                    continue;
                }
                if (vc == ';') {
                    break;
                }
                pos += 1;
            }
            if (v_quote != 0) {
                return err.UnclosedBlock;
            }
            const val_raw = trimWs(body[val_start..pos]);
            if (pos < body.len && body[pos] == ';') {
                pos += 1;
            }

            if (eqlAsciiIgnoreCase(prop_name, "system")) {
                rule.system = parseSystemDescriptor(val_raw)?;
            } else if (eqlAsciiIgnoreCase(prop_name, "symbols")) {
                parseSymbolsDescriptor(&rule, val_raw)?;
            } else if (eqlAsciiIgnoreCase(prop_name, "additive-symbols")) {
                parseAdditiveSymbolsDescriptor(&rule, val_raw)?;
            } else if (eqlAsciiIgnoreCase(prop_name, "negative")) {
                parseNegativeDescriptor(&rule, val_raw)?;
            } else if (eqlAsciiIgnoreCase(prop_name, "prefix")) {
                if (tok := nextSymbolToken(val_raw, 0)) {
                    if (skipWsAndComments(val_raw, tok.next_pos) != val_raw.len) {
                        return err.InvalidPrelude;
                    }
                    rule.prefix = tok.value;
                    rule.has_prefix = true;
                } else {
                    return err.InvalidPrelude;
                }
            } else if (eqlAsciiIgnoreCase(prop_name, "suffix")) {
                if (tok := nextSymbolToken(val_raw, 0)) {
                    if (skipWsAndComments(val_raw, tok.next_pos) != val_raw.len) {
                        return err.InvalidPrelude;
                    }
                    rule.suffix = tok.value;
                    rule.has_suffix = true;
                } else {
                    return err.InvalidPrelude;
                }
            } else if (eqlAsciiIgnoreCase(prop_name, "range")) {
                parseRangeDescriptor(&rule, val_raw)?;
            } else if (eqlAsciiIgnoreCase(prop_name, "pad")) {
                parsePadDescriptor(&rule, val_raw)?;
            } else if (eqlAsciiIgnoreCase(prop_name, "fallback")) {
                if (!isValidCustomIdent(val_raw) || isForbiddenCounterName(val_raw)) {
                    return err.InvalidPrelude;
                }
                rule.fallback = val_raw;
                rule.has_fallback = true;
            } else if (eqlAsciiIgnoreCase(prop_name, "speak-as")) {
                rule.speak_as = parseSpeakAsDescriptor(val_raw)?;
                rule.has_speak_as = true;
            }
        }

        // W3C 规范跨描述符一致性校验
        if (rule.system.kind == .extends) {
            if (rule.symbol_count > 0 || rule.additive_count > 0) {
                return err.InvalidSystem;
            }
        } else if (rule.system.kind == .additive) {
            if (rule.additive_count == 0) {
                return err.InvalidAdditiveSymbols;
            }
        } else if (rule.system.kind == .numeric || rule.system.kind == .alphabetic) {
            if (rule.symbol_count < 2) {
                return err.InvalidSymbols;
            }
        } else {
            if (rule.symbol_count == 0) {
                return err.InvalidSymbols;
            }
        }

        return rule;
    }

    /// 判断整数是否处于本规则有效范围内（`range_count == 0` 时按 W3C `auto` 规则判定）
    fn isInRange(value: i32): bool {
        if (_.range_count == 0) {
            if (_.system.kind == .alphabetic || _.system.kind == .symbolic) {
                return value >= 1;
            }
            if (_.system.kind == .additive) {
                return value >= 0;
            }
            return true;
        }
        let i: usize = 0;
        while (i < _.range_count) {
            if (_.ranges[i].contains(value)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 按 W3C CSS Counter Styles Level 3 算法将计数器值格式化为纯表示字符串（含 `negative` 与 `pad`，不含 `prefix`/`suffix`）
    fn formatCounter(value: i32, out: []u8): usize! {
        if (!_.isInRange(value)) {
            return err.OutOfRange;
        }
        if (_.system.kind == .extends) {
            return err.InvalidSystem;
        }

        const use_negative = value < 0 &&
            (_.system.kind == .numeric || _.system.kind == .alphabetic || _.system.kind == .symbolic || _.system.kind == .additive);

        let mag: i64 = value as i64;
        if (use_negative) {
            mag = -mag;
        }

        let core_buf: [256]u8 = [0; 256];
        let core_len: usize = 0;

        if (_.system.kind == .cyclic) {
            const n = _.symbol_count as i64;
            let rem = ((value as i64) - 1) % n;
            if (rem < 0) {
                rem += n;
            }
            core_len = writeBytes(core_buf[0..], 0, _.symbols[rem as usize])?;
        } else if (_.system.kind == .fixed) {
            const offset = (value as i64) - (_.system.fixed_first as i64);
            if (offset < 0 || (offset as usize) >= _.symbol_count) {
                return err.OutOfRange;
            }
            core_len = writeBytes(core_buf[0..], 0, _.symbols[offset as usize])?;
        } else if (_.system.kind == .symbolic) {
            if (mag < 1) {
                return err.OutOfRange;
            }
            const n = _.symbol_count as i64;
            const sym_idx = ((mag - 1) % n) as usize;
            const repeat = (((mag - 1) / n) + 1) as usize;
            let r: usize = 0;
            while (r < repeat) {
                core_len = writeBytes(core_buf[0..], core_len, _.symbols[sym_idx])?;
                r += 1;
            }
        } else if (_.system.kind == .alphabetic) {
            if (mag < 1) {
                return err.OutOfRange;
            }
            const n = _.symbol_count as i64;
            let indices: [64]usize = [0; 64];
            let count: usize = 0;
            let cur = mag;
            while (cur > 0) {
                cur -= 1;
                if (count >= 64) {
                    return err.BufferTooSmall;
                }
                indices[count] = (cur % n) as usize;
                cur = cur / n;
                count += 1;
            }
            while (count > 0) {
                count -= 1;
                core_len = writeBytes(core_buf[0..], core_len, _.symbols[indices[count]])?;
            }
        } else if (_.system.kind == .numeric) {
            const n = _.symbol_count as i64;
            if (mag == 0) {
                core_len = writeBytes(core_buf[0..], 0, _.symbols[0])?;
            } else {
                let indices: [64]usize = [0; 64];
                let count: usize = 0;
                let cur = mag;
                while (cur > 0) {
                    if (count >= 64) {
                        return err.BufferTooSmall;
                    }
                    indices[count] = (cur % n) as usize;
                    cur = cur / n;
                    count += 1;
                }
                while (count > 0) {
                    count -= 1;
                    core_len = writeBytes(core_buf[0..], core_len, _.symbols[indices[count]])?;
                }
            }
        } else if (_.system.kind == .additive) {
            if (mag == 0) {
                const last = _.additive_symbols[_.additive_count - 1];
                if (last.weight != 0) {
                    return err.OutOfRange;
                }
                core_len = writeBytes(core_buf[0..], 0, last.symbol)?;
            } else {
                let rem = mag;
                let idx: usize = 0;
                while (idx < _.additive_count && rem > 0) {
                    const w = _.additive_symbols[idx].weight as i64;
                    if (w == 0) {
                        idx += 1;
                        continue;
                    }
                    if (w <= rem) {
                        const times = (rem / w) as usize;
                        let t: usize = 0;
                        while (t < times) {
                            core_len = writeBytes(core_buf[0..], core_len, _.additive_symbols[idx].symbol)?;
                            t += 1;
                        }
                        rem = rem % w;
                    }
                    idx += 1;
                }
                if (rem != 0) {
                    return err.OutOfRange;
                }
            }
        }

        // 计算 pad 补齐次数（按 W3C 规范：负号计入最小宽度）
        let pad_reps: usize = 0;
        if (_.has_pad && _.pad_width > 0) {
            let cur_cp = countUtf8Codepoints(core_buf[0..core_len]);
            if (use_negative) {
                cur_cp += countUtf8Codepoints(_.negative_prefix) + countUtf8Codepoints(_.negative_suffix);
            }
            if (cur_cp < _.pad_width) {
                pad_reps = _.pad_width - cur_cp;
            }
        }

        let pos: usize = 0;
        if (use_negative) {
            pos = writeBytes(out, pos, _.negative_prefix)?;
        }
        let p: usize = 0;
        while (p < pad_reps) {
            pos = writeBytes(out, pos, _.pad_symbol)?;
            p += 1;
        }
        pos = writeBytes(out, pos, core_buf[0..core_len])?;
        if (use_negative) {
            pos = writeBytes(out, pos, _.negative_suffix)?;
        }
        return pos;
    }

    /// 格式化完整列表项标记：`prefix + formatCounter(value) + suffix`
    fn formatMarker(value: i32, out: []u8): usize! {
        let pos: usize = 0;
        if (_.prefix.len > 0) {
            pos = writeBytes(out, pos, _.prefix)?;
        }
        const cn = _.formatCounter(value, out[pos..])?;
        pos += cn;
        if (_.suffix.len > 0) {
            pos = writeBytes(out, pos, _.suffix)?;
        }
        return pos;
    }

    /// 规范序列化为 `@counter-style <name> { ... }`（支持 Round-Trip 幂等）
    fn toCss(out: []u8): usize! {
        let pos = writeBytes(out, 0, "@counter-style ")?;
        pos = writeBytes(out, pos, _.name)?;
        pos = writeBytes(out, pos, " { system: ")?;
        if (_.system.kind == .symbolic) {
            pos = writeBytes(out, pos, "symbolic")?;
        } else if (_.system.kind == .cyclic) {
            pos = writeBytes(out, pos, "cyclic")?;
        } else if (_.system.kind == .numeric) {
            pos = writeBytes(out, pos, "numeric")?;
        } else if (_.system.kind == .alphabetic) {
            pos = writeBytes(out, pos, "alphabetic")?;
        } else if (_.system.kind == .additive) {
            pos = writeBytes(out, pos, "additive")?;
        } else if (_.system.kind == .fixed) {
            pos = writeBytes(out, pos, "fixed")?;
            if (_.system.fixed_first != 1) {
                pos = writeByte(out, pos, ' ')?;
                pos = writeI32(out, pos, _.system.fixed_first)?;
            }
        } else if (_.system.kind == .extends) {
            pos = writeBytes(out, pos, "extends ")?;
            pos = writeBytes(out, pos, _.system.extends_name)?;
        }
        pos = writeByte(out, pos, ';')?;

        if (_.symbol_count > 0) {
            pos = writeBytes(out, pos, " symbols:")?;
            let i: usize = 0;
            while (i < _.symbol_count) {
                pos = writeByte(out, pos, ' ')?;
                pos = writeCssQuoted(out, pos, _.symbols[i])?;
                i += 1;
            }
            pos = writeByte(out, pos, ';')?;
        }

        if (_.additive_count > 0) {
            pos = writeBytes(out, pos, " additive-symbols:")?;
            let i: usize = 0;
            while (i < _.additive_count) {
                if (i > 0) {
                    pos = writeByte(out, pos, ',')?;
                }
                pos = writeByte(out, pos, ' ')?;
                pos = writeI32(out, pos, _.additive_symbols[i].weight)?;
                pos = writeByte(out, pos, ' ')?;
                pos = writeCssQuoted(out, pos, _.additive_symbols[i].symbol)?;
                i += 1;
            }
            pos = writeByte(out, pos, ';')?;
        }

        if (_.has_negative) {
            pos = writeBytes(out, pos, " negative: ")?;
            pos = writeCssQuoted(out, pos, _.negative_prefix)?;
            if (_.negative_suffix.len > 0) {
                pos = writeByte(out, pos, ' ')?;
                pos = writeCssQuoted(out, pos, _.negative_suffix)?;
            }
            pos = writeByte(out, pos, ';')?;
        }

        if (_.has_prefix) {
            pos = writeBytes(out, pos, " prefix: ")?;
            pos = writeCssQuoted(out, pos, _.prefix)?;
            pos = writeByte(out, pos, ';')?;
        }

        if (_.has_suffix) {
            pos = writeBytes(out, pos, " suffix: ")?;
            pos = writeCssQuoted(out, pos, _.suffix)?;
            pos = writeByte(out, pos, ';')?;
        }

        if (_.range_count > 0) {
            pos = writeBytes(out, pos, " range:")?;
            let i: usize = 0;
            while (i < _.range_count) {
                if (i > 0) {
                    pos = writeByte(out, pos, ',')?;
                }
                pos = writeByte(out, pos, ' ')?;
                if (_.ranges[i].has_min) {
                    pos = writeI32(out, pos, _.ranges[i].min_val)?;
                } else {
                    pos = writeBytes(out, pos, "infinite")?;
                }
                pos = writeByte(out, pos, ' ')?;
                if (_.ranges[i].has_max) {
                    pos = writeI32(out, pos, _.ranges[i].max_val)?;
                } else {
                    pos = writeBytes(out, pos, "infinite")?;
                }
                i += 1;
            }
            pos = writeByte(out, pos, ';')?;
        }

        if (_.has_pad) {
            pos = writeBytes(out, pos, " pad: ")?;
            pos = writeI32(out, pos, _.pad_width as i32)?;
            pos = writeByte(out, pos, ' ')?;
            pos = writeCssQuoted(out, pos, _.pad_symbol)?;
            pos = writeByte(out, pos, ';')?;
        }

        if (_.has_fallback) {
            pos = writeBytes(out, pos, " fallback: ")?;
            pos = writeBytes(out, pos, _.fallback)?;
            pos = writeByte(out, pos, ';')?;
        }

        if (_.has_speak_as) {
            pos = writeBytes(out, pos, " speak-as: ")?;
            if (_.speak_as == .auto_mode) {
                pos = writeBytes(out, pos, "auto")?;
            } else if (_.speak_as == .bullets) {
                pos = writeBytes(out, pos, "bullets")?;
            } else if (_.speak_as == .numbers) {
                pos = writeBytes(out, pos, "numbers")?;
            } else if (_.speak_as == .words) {
                pos = writeBytes(out, pos, "words")?;
            } else if (_.speak_as == .spell_out) {
                pos = writeBytes(out, pos, "spell-out")?;
            }
            pos = writeByte(out, pos, ';')?;
        }

        pos = writeBytes(out, pos, " }")?;
        return pos;
    }

    /// 语义等价比较
    fn eql(other: CssCounterStyleRule): bool {
        if (!bytesEql(_.name, other.name) || !_.system.eql(other.system)) {
            return false;
        }
        if (_.symbol_count != other.symbol_count || _.additive_count != other.additive_count || _.range_count != other.range_count) {
            return false;
        }
        let i: usize = 0;
        while (i < _.symbol_count) {
            if (!bytesEql(_.symbols[i], other.symbols[i])) {
                return false;
            }
            i += 1;
        }
        i = 0;
        while (i < _.additive_count) {
            if (_.additive_symbols[i].weight != other.additive_symbols[i].weight || !bytesEql(_.additive_symbols[i].symbol, other.additive_symbols[i].symbol)) {
                return false;
            }
            i += 1;
        }
        i = 0;
        while (i < _.range_count) {
            if (!_.ranges[i].eql(other.ranges[i])) {
                return false;
            }
            i += 1;
        }
        return bytesEql(_.negative_prefix, other.negative_prefix) &&
            bytesEql(_.negative_suffix, other.negative_suffix) &&
            bytesEql(_.prefix, other.prefix) &&
            bytesEql(_.suffix, other.suffix) &&
            _.pad_width == other.pad_width &&
            bytesEql(_.pad_symbol, other.pad_symbol) &&
            bytesEql(_.fallback, other.fallback) &&
            _.speak_as == other.speak_as;
    }
}

fn parseSystemDescriptor(val: []const u8): CssCounterSystem! {
    const t1 = nextSymbolToken(val, 0);
    if (tok1 := t1) {
        if (tok1.was_quoted) {
            return err.InvalidSystem;
        }
        const rest_pos = skipWsAndComments(val, tok1.next_pos);
        if (eqlAsciiIgnoreCase(tok1.value, "cyclic")) {
            if (rest_pos != val.len) return err.InvalidSystem;
            return CssCounterSystem{ kind: .cyclic, fixed_first: 1, extends_name: "" };
        }
        if (eqlAsciiIgnoreCase(tok1.value, "numeric")) {
            if (rest_pos != val.len) return err.InvalidSystem;
            return CssCounterSystem{ kind: .numeric, fixed_first: 1, extends_name: "" };
        }
        if (eqlAsciiIgnoreCase(tok1.value, "alphabetic")) {
            if (rest_pos != val.len) return err.InvalidSystem;
            return CssCounterSystem{ kind: .alphabetic, fixed_first: 1, extends_name: "" };
        }
        if (eqlAsciiIgnoreCase(tok1.value, "symbolic")) {
            if (rest_pos != val.len) return err.InvalidSystem;
            return CssCounterSystem{ kind: .symbolic, fixed_first: 1, extends_name: "" };
        }
        if (eqlAsciiIgnoreCase(tok1.value, "additive")) {
            if (rest_pos != val.len) return err.InvalidSystem;
            return CssCounterSystem{ kind: .additive, fixed_first: 1, extends_name: "" };
        }
        if (eqlAsciiIgnoreCase(tok1.value, "fixed")) {
            if (rest_pos == val.len) {
                return CssCounterSystem.fixedFrom(1);
            }
            if (tok2 := nextSymbolToken(val, rest_pos)) {
                if (tok2.was_quoted || skipWsAndComments(val, tok2.next_pos) != val.len) {
                    return err.InvalidSystem;
                }
                if (first_val := parseI32(tok2.value)) {
                    return CssCounterSystem.fixedFrom(first_val);
                }
            }
            return err.InvalidSystem;
        }
        if (eqlAsciiIgnoreCase(tok1.value, "extends")) {
            if (tok2 := nextSymbolToken(val, rest_pos)) {
                if (tok2.was_quoted || skipWsAndComments(val, tok2.next_pos) != val.len) {
                    return err.InvalidSystem;
                }
                if (!isValidCustomIdent(tok2.value) || isForbiddenCounterName(tok2.value)) {
                    return err.InvalidSystem;
                }
                return CssCounterSystem{
                    kind: .extends,
                    fixed_first: 1,
                    extends_name: tok2.value,
                };
            }
            return err.InvalidSystem;
        }
    }
    return err.InvalidSystem;
}

fn parseSymbolsDescriptor(rule: *CssCounterStyleRule, val: []const u8): usize! {
    rule.symbol_count = 0;
    let pos: usize = 0;
    while (pos < val.len) {
        pos = skipWsAndComments(val, pos);
        if (pos >= val.len) {
            break;
        }
        if (tok := nextSymbolToken(val, pos)) {
            if (rule.symbol_count >= CSS_COUNTER_MAX_SYMBOLS) {
                return err.TooManySymbols;
            }
            rule.symbols[rule.symbol_count] = tok.value;
            rule.symbol_count += 1;
            pos = tok.next_pos;
        } else {
            return err.InvalidSymbols;
        }
    }
    if (rule.symbol_count == 0) {
        return err.InvalidSymbols;
    }
    return rule.symbol_count;
}

fn parseAdditiveSymbolsDescriptor(rule: *CssCounterStyleRule, val: []const u8): usize! {
    rule.additive_count = 0;
    let pos: usize = 0;
    let prev_weight: i32 = 0;
    let has_prev = false;

    while (pos < val.len) {
        pos = skipWsAndComments(val, pos);
        if (pos >= val.len) {
            break;
        }
        const t1_opt = nextSymbolToken(val, pos);
        if (t1 := t1_opt) {
            const t2_opt = nextSymbolToken(val, t1.next_pos);
            if (t2 := t2_opt) {
                // W3C 允许 `<integer> <symbol>` 或 `<symbol> <integer>`
                let weight: i32 = 0;
                let sym: []const u8 = "";
                if (!t1.was_quoted) {
                    if (w1 := parseI32(t1.value)) {
                        weight = w1;
                        sym = t2.value;
                    } else if (!t2.was_quoted) {
                        if (w2 := parseI32(t2.value)) {
                            weight = w2;
                            sym = t1.value;
                        } else {
                            return err.InvalidAdditiveSymbols;
                        }
                    } else {
                        return err.InvalidAdditiveSymbols;
                    }
                } else if (!t2.was_quoted) {
                    if (w2 := parseI32(t2.value)) {
                        weight = w2;
                        sym = t1.value;
                    } else {
                        return err.InvalidAdditiveSymbols;
                    }
                } else {
                    return err.InvalidAdditiveSymbols;
                }

                // 权重必须非负且严格降序
                if (weight < 0) {
                    return err.InvalidAdditiveSymbols;
                }
                if (has_prev && weight >= prev_weight) {
                    return err.InvalidAdditiveSymbols;
                }
                if (rule.additive_count >= CSS_COUNTER_MAX_SYMBOLS) {
                    return err.TooManySymbols;
                }
                rule.additive_symbols[rule.additive_count] = CssAdditiveTuple{
                    weight: weight,
                    symbol: sym,
                };
                rule.additive_count += 1;
                prev_weight = weight;
                has_prev = true;

                pos = skipWsAndComments(val, t2.next_pos);
                if (pos < val.len) {
                    if (val[pos] != ',') {
                        return err.InvalidAdditiveSymbols;
                    }
                    pos += 1;
                    if (skipWsAndComments(val, pos) >= val.len) {
                        return err.InvalidAdditiveSymbols;
                    }
                }
            } else {
                return err.InvalidAdditiveSymbols;
            }
        } else {
            return err.InvalidAdditiveSymbols;
        }
    }

    if (rule.additive_count == 0) {
        return err.InvalidAdditiveSymbols;
    }
    return rule.additive_count;
}

fn parseNegativeDescriptor(rule: *CssCounterStyleRule, val: []const u8): usize! {
    if (t1 := nextSymbolToken(val, 0)) {
        rule.negative_prefix = t1.value;
        const p2 = skipWsAndComments(val, t1.next_pos);
        if (p2 == val.len) {
            rule.negative_suffix = "";
            rule.has_negative = true;
            return 1;
        }
        if (t2 := nextSymbolToken(val, p2)) {
            if (skipWsAndComments(val, t2.next_pos) != val.len) {
                return err.InvalidPrelude;
            }
            rule.negative_suffix = t2.value;
            rule.has_negative = true;
            return 2;
        }
    }
    return err.InvalidPrelude;
}

fn parseRangeDescriptor(rule: *CssCounterStyleRule, val: []const u8): usize! {
    rule.range_count = 0;
    if (eqlAsciiIgnoreCase(val, "auto")) {
        return 0;
    }
    let pos: usize = 0;
    while (pos < val.len) {
        pos = skipWsAndComments(val, pos);
        if (pos >= val.len) {
            break;
        }
        const t1_opt = nextSymbolToken(val, pos);
        if (t1 := t1_opt) {
            const t2_opt = nextSymbolToken(val, t1.next_pos);
            if (t2 := t2_opt) {
                if (t1.was_quoted || t2.was_quoted) {
                    return err.InvalidRange;
                }
                let has_min = false;
                let min_val: i32 = 0;
                if (!eqlAsciiIgnoreCase(t1.value, "infinite")) {
                    if (v1 := parseI32(t1.value)) {
                        has_min = true;
                        min_val = v1;
                    } else {
                        return err.InvalidRange;
                    }
                }
                let has_max = false;
                let max_val: i32 = 0;
                if (!eqlAsciiIgnoreCase(t2.value, "infinite")) {
                    if (v2 := parseI32(t2.value)) {
                        has_max = true;
                        max_val = v2;
                    } else {
                        return err.InvalidRange;
                    }
                }
                if (has_min && has_max && min_val > max_val) {
                    return err.InvalidRange;
                }
                if (rule.range_count >= CSS_COUNTER_MAX_RANGES) {
                    return err.TooManyRanges;
                }
                rule.ranges[rule.range_count] = CssCounterRange{
                    has_min: has_min,
                    min_val: min_val,
                    has_max: has_max,
                    max_val: max_val,
                };
                rule.range_count += 1;

                pos = skipWsAndComments(val, t2.next_pos);
                if (pos < val.len) {
                    if (val[pos] != ',') {
                        return err.InvalidRange;
                    }
                    pos += 1;
                    if (skipWsAndComments(val, pos) >= val.len) {
                        return err.InvalidRange;
                    }
                }
            } else {
                return err.InvalidRange;
            }
        } else {
            return err.InvalidRange;
        }
    }
    if (rule.range_count == 0) {
        return err.InvalidRange;
    }
    return rule.range_count;
}

fn parsePadDescriptor(rule: *CssCounterStyleRule, val: []const u8): usize! {
    const t1_opt = nextSymbolToken(val, 0);
    if (t1 := t1_opt) {
        const t2_opt = nextSymbolToken(val, t1.next_pos);
        if (t2 := t2_opt) {
            if (skipWsAndComments(val, t2.next_pos) != val.len) {
                return err.InvalidPad;
            }
            // W3C 允许 `<integer> && <symbol>` 任意顺序
            if (!t1.was_quoted) {
                if (w1 := parseI32(t1.value)) {
                    if (w1 < 0) return err.InvalidPad;
                    rule.pad_width = w1 as usize;
                    rule.pad_symbol = t2.value;
                    rule.has_pad = true;
                    return rule.pad_width;
                }
            }
            if (!t2.was_quoted) {
                if (w2 := parseI32(t2.value)) {
                    if (w2 < 0) return err.InvalidPad;
                    rule.pad_width = w2 as usize;
                    rule.pad_symbol = t1.value;
                    rule.has_pad = true;
                    return rule.pad_width;
                }
            }
        }
    }
    return err.InvalidPad;
}

fn parseSpeakAsDescriptor(val: []const u8): CssCounterSpeakAs! {
    if (eqlAsciiIgnoreCase(val, "auto")) return .auto_mode;
    if (eqlAsciiIgnoreCase(val, "bullets")) return .bullets;
    if (eqlAsciiIgnoreCase(val, "numbers")) return .numbers;
    if (eqlAsciiIgnoreCase(val, "words")) return .words;
    if (eqlAsciiIgnoreCase(val, "spell-out")) return .spell_out;
    return err.InvalidSpeakAs;
}
