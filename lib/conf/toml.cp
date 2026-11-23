// Reference-hardened against toml-rs/toml (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/interchange/toml/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/conf/toml.cp — 工业级 TOML v1.0.0 严格合规解析器、AST 构建器与双向序列化器
// 对标 toml-rs/toml (MIT/Apache-2.0) 加固：
//   - 严格键与表语义：裸键/引号键/点分键、重复键拒绝、显式表重定义拒绝、
//     Inline Table 冻结不可追加（禁止换行与尾随逗号）、[[array_of_tables]] vs 静态数组隔离、
//     禁止多行字符串作为键、64 层最大嵌套深度防护
//   - 严格字符串与转义：基础/多行基础/字面量/多行字面量字符串、\uXXXX/\UXXXXXXXX 代理对拒绝
//     (0xD800..0xDFFF & >0x10FFFF)、禁止未转义控制字符与孤立 CR
//   - 严格数值与日期时间：禁止前导零、下划线位置校验、0x/0o/0b 小写无符号进制整数、
//     inf/nan 大小写敏感、有限浮点溢出拒绝、RFC 3339 闰年与日历时间边界校验
//   - 热路径 cap:simd 加速：注释跳行、空白跳过与单行/多行字符串块级扫描
//   - 100% 向后兼容 TomlDoc (TOML / toml) 与 TomlParser -> ConfValue
// ============================================================================

const std = @import("std");
import { ConfTag, ConfDateTime, ConfField, ConfValue } from "./value";
import { Simd } from "cap:simd";

/// TOML 最大嵌套深度（防畸形深层 `[[[[...` 或 `{a={b={...` 栈溢出）
const TOML_MAX_DEPTH: usize = 64;

pub error TomlError {
    OutOfMemory,
    UnexpectedEof,
    SyntaxError,
    DuplicateKey,
    TableRedefined,
    InvalidEscape,
    InvalidNumber,
    InvalidDateTime,
    DepthExceeded,
    InvalidControlChar,
    FrozenTable,
}

type TomlByteList = std.ArrayListUnmanaged(u8);
type TomlBuildFieldList = std.ArrayListUnmanaged(BuildField);
type TomlUsizeList = std.ArrayListUnmanaged(usize);
type TomlBuildNodeList = std.ArrayListUnmanaged(BuildNode);

/// 内部表/数组语义状态（严格对齐 TOML v1.0.0 表定义规则）
enum NodeKind: u8 {
    Scalar = 0,
    TableImplicit = 1,
    TableExplicit = 2,
    TableDotted = 3,
    TableInlineFrozen = 4,
    ArrayStatic = 5,
    ArrayOfTables = 6,
}

struct BuildField {
    key: []const u8 = "",
    node_id: usize = 0,
}

struct BuildNode {
    kind: NodeKind = NodeKind.Scalar,
    scalar: ConfValue = ConfValue{},
    fields: TomlBuildFieldList = TomlBuildFieldList.empty,
    items: TomlUsizeList = TomlUsizeList.empty,
    /// 对于 TableDotted，记录当前表头作用域 ID；离开作用域后不可再用点分键跨表扩展
    scope_id: usize = 0,

    static fn isTableKind(kind: NodeKind): bool {
        return kind == NodeKind.TableImplicit ||
            kind == NodeKind.TableExplicit ||
            kind == NodeKind.TableDotted ||
            kind == NodeKind.TableInlineFrozen;
    }
}

pub struct TomlEntry {
    pub table: []const u8 = "";
    pub key: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct TomlParser {
    src: []const u8 = "",
    pos: usize = 0,
    allocator: std.mem.Allocator = undefined,
    scope_counter: usize = 0,
    nodes: TomlBuildNodeList = TomlBuildNodeList.empty,

    pub static fn parse(allocator: std.mem.Allocator, content: []const u8): ConfValue! {
        let p = TomlParser{
            src: content,
            pos: 0,
            allocator: allocator,
            scope_counter: 1,
            nodes: TomlBuildNodeList.empty,
        };
        defer p.deinitNodes();
        return try p.runParser();
    }

    fn deinitNodes(*_) {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            _.nodes.items[i].fields.deinit(_.allocator);
            _.nodes.items[i].items.deinit(_.allocator);
        }
        _.nodes.deinit(_.allocator);
    }

    fn allocNode(*_, node: BuildNode): usize! {
        const id = _.nodes.items.len;
        try _.nodes.append(_.allocator, node);
        return id;
    }

    fn nodeToConfValue(*_, node_id: usize): ConfValue! {
        const k = _.nodes.items[node_id].kind;
        if (k == NodeKind.Scalar) {
            return _.nodes.items[node_id].scalar;
        }
        if (k == NodeKind.ArrayStatic || k == NodeKind.ArrayOfTables) {
            const count = _.nodes.items[node_id].items.items.len;
            let arr = try _.allocator.alloc(ConfValue, count);
            let i: usize = 0;
            while (i < count) : (i += 1) {
                const child_id = _.nodes.items[node_id].items.items[i];
                arr[i] = try _.nodeToConfValue(child_id);
            }
            return ConfValue.makeArray(arr);
        }
        const fcount = _.nodes.items[node_id].fields.items.len;
        let fields = try _.allocator.alloc(ConfField, fcount);
        let i: usize = 0;
        while (i < fcount) : (i += 1) {
            const key = _.nodes.items[node_id].fields.items[i].key;
            const child_id = _.nodes.items[node_id].fields.items[i].node_id;
            fields[i] = ConfField{
                key: key,
                value: try _.nodeToConfValue(child_id),
            };
        }
        return ConfValue.makeObject(fields);
    }

    /// 检查字符是否为 TOML 禁止的注释/单行字符串控制字符（允许 TAB 0x09，禁止 0x00..0x08, 0x0A..0x1F, 0x7F）
    static fn isDisallowedControlSingleLine(c: u8): bool {
        return (c <= 0x08) || (c >= 0x0A && c <= 0x1F) || c == 0x7F;
    }

    /// 检查字符是否为多行字符串禁止的控制字符（允许 TAB 0x09, LF 0x0A, CR 0x0D，禁止其余控制字符）
    static fn isDisallowedControlMultiLine(c: u8): bool {
        return (c <= 0x08) || c == 0x0B || c == 0x0C || (c >= 0x0E && c <= 0x1F) || c == 0x7F;
    }

    /// 校验一段切片内不含单行禁止控制字符
    static fn validateNoControlSingleLine(slice: []const u8): ! {
        let i: usize = 0;
        while (i < slice.len) : (i += 1) {
            if (TomlParser.isDisallowedControlSingleLine(slice[i])) {
                return error.InvalidControlChar;
            }
        }
    }

    /// 校验一段多行切片内不含禁止控制字符及孤立 CR
    static fn validateNoControlMultiLine(slice: []const u8): ! {
        let i: usize = 0;
        while (i < slice.len) : (i += 1) {
            const c = slice[i];
            if (TomlParser.isDisallowedControlMultiLine(c)) {
                return error.InvalidControlChar;
            }
            if (c == '\r') {
                if (i + 1 >= slice.len || slice[i + 1] != '\n') {
                    return error.InvalidControlChar;
                }
            }
        }
    }

    /// SIMD 热路径：在基础字符串内容中查找下一个 '"', '\\', '\n', '\r' 偏移（供测试与标量对拍）
    pub static fn scanBasicStringStopSimd(slice: []const u8): usize? {
        return Simd.indexOfAny4(slice, '"', '\\', '\n', '\r');
    }

    /// 标量参考：在基础字符串内容中查找下一个 '"', '\\', '\n', '\r' 偏移
    pub static fn scanBasicStringStopScalar(slice: []const u8): usize? {
        let i: usize = 0;
        while (i < slice.len) : (i += 1) {
            const c = slice[i];
            if (c == '"' || c == '\\' || c == '\n' || c == '\r') {
                return i;
            }
        }
        return null;
    }

    fn skipWs(*_) {
        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
            _.pos += 1;
        }
    }

    /// 使用 cap:simd 跳过 `# ...` 单行注释并校验注释内无非法控制字符
    fn skipCommentBody(*_): ! {
        // _.src[_.pos] == '#'
        _.pos += 1;
        const rest = _.src[_.pos..];
        let comment_len: usize = rest.len;
        if (nl := Simd.indexOfByte(rest, '\n')) {
            comment_len = nl;
        }
        // 允许行尾紧贴 '\n' 之前的单个 '\r' (CRLF)
        let check_len = comment_len;
        if (check_len > 0 && rest[check_len - 1] == '\r' && comment_len < rest.len) {
            check_len -= 1;
        }
        try TomlParser.validateNoControlSingleLine(rest[0..check_len]);
        _.pos += comment_len;
    }

    fn skipWsAndComments(*_): ! {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                if (c == '\r') {
                    if (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '\n') {
                        return error.InvalidControlChar;
                    }
                    _.pos += 2;
                } else {
                    const adv = Simd.skipWhitespace(_.src[_.pos..]);
                    // 仅对连续不含孤立 CR 的空白块批量前进；若块内含 '\r' 则逐字节检查 CRLF
                    if (adv > 1 && Simd.indexOfByte(_.src[_.pos .. _.pos + adv], '\r') == null) {
                        _.pos += adv;
                    } else {
                        _.pos += 1;
                    }
                }
            } else if (c == '#') {
                try _.skipCommentBody();
            } else {
                break;
            }
        }
    }

    fn skipCommentOnLine(*_): ! {
        _.skipWs();
        if (_.pos < _.src.len && _.src[_.pos] == '#') {
            try _.skipCommentBody();
        }
    }

    fn expectLineEndOrEof(*_): ! {
        try _.skipCommentOnLine();
        if (_.pos >= _.src.len) return;
        if (_.src[_.pos] == '\r') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
                _.pos += 2;
                return;
            }
            return error.InvalidControlChar;
        }
        if (_.src[_.pos] == '\n') {
            _.pos += 1;
            return;
        }
        return error.SyntaxError;
    }

    static fn isBareKeyChar(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-';
    }

    /// 解析单个键段（TOML v1.0.0：允许裸键、单行基础字符串、单行字面量字符串；严禁多行字符串作为键）
    fn parseKeySegment(*_): ([]const u8)! {
        _.skipWs();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const c = _.src[_.pos];
        if (c == '"') {
            if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
                return error.SyntaxError;
            }
            return try _.parseBasicStringSingleLine();
        }
        if (c == '\'') {
            if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '\'' && _.src[_.pos + 2] == '\'') {
                return error.SyntaxError;
            }
            return try _.parseLiteralStringSingleLine();
        }
        const start = _.pos;
        while (_.pos < _.src.len && TomlParser.isBareKeyChar(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos == start) return error.SyntaxError;
        return _.src[start.._.pos];
    }

    fn parseKeyPath(*_, seg_buf: *[32][]const u8): usize! {
        let count: usize = 0;
        const first = try _.parseKeySegment();
        seg_buf[0] = first;
        count = 1;
        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '.') break;
            _.pos += 1;
            if (count >= 32) return error.DepthExceeded;
            seg_buf[count] = try _.parseKeySegment();
            count += 1;
        }
        return count;
    }

    static fn parseHex(s: []const u8): u32? {
        let acc: u32 = 0;
        for (c of s) {
            acc <<= 4;
            if (c >= '0' && c <= '9') {
                acc |= (c - '0') as u32;
            } else if (c >= 'a' && c <= 'f') {
                acc |= ((c - 'a') + 10) as u32;
            } else if (c >= 'A' && c <= 'F') {
                acc |= ((c - 'A') + 10) as u32;
            } else {
                return null;
            }
        }
        return acc;
    }

    /// 严格 UTF-8 标量编码（拒绝 UTF-16 代理对 0xD800..0xDFFF 及 > 0x10FFFF）
    static fn appendUtf8(out: *TomlByteList, allocator: std.mem.Allocator, cp: u32): ! {
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return error.InvalidEscape;
        }
        let buf: [4]u8 = undefined;
        const n = std.unicode.utf8Encode(@intCast(cp), &buf) catch return error.InvalidEscape;
        try out.appendSlice(allocator, buf[0..n]);
    }

    fn parseBasicEscape(*_, out: *TomlByteList): ! {
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const esc = _.src[_.pos];
        _.pos += 1;
        switch (esc) {
            'b' => try out.append(_.allocator, 0x08),
            't' => try out.append(_.allocator, '\t'),
            'n' => try out.append(_.allocator, '\n'),
            'f' => try out.append(_.allocator, 0x0C),
            'r' => try out.append(_.allocator, '\r'),
            '"' => try out.append(_.allocator, '"'),
            '\\' => try out.append(_.allocator, '\\'),
            'u' => {
                if (_.pos + 4 > _.src.len) return error.InvalidEscape;
                const cp = TomlParser.parseHex(_.src[_.pos .. _.pos + 4]) orelse return error.InvalidEscape;
                _.pos += 4;
                try TomlParser.appendUtf8(out, _.allocator, cp);
            },
            'U' => {
                if (_.pos + 8 > _.src.len) return error.InvalidEscape;
                const cp = TomlParser.parseHex(_.src[_.pos .. _.pos + 8]) orelse return error.InvalidEscape;
                _.pos += 8;
                try TomlParser.appendUtf8(out, _.allocator, cp);
            },
            else => return error.InvalidEscape,
        }
    }

    /// 单行基础字符串（使用 cap:simd Simd.indexOfAny4 块级快扫）
    fn parseBasicStringSingleLine(*_): ([]const u8)! {
        _.pos += 1; // '"'
        let out = TomlByteList.empty;
        errdefer out.deinit(_.allocator);
        while (_.pos < _.src.len) {
            const rest = _.src[_.pos..];
            const stop_opt = TomlParser.scanBasicStringStopSimd(rest);
            if (stop_opt == null) {
                return error.UnexpectedEof;
            }
            const stop = stop_opt.?;
            if (stop > 0) {
                const chunk = rest[0..stop];
                try TomlParser.validateNoControlSingleLine(chunk);
                try out.appendSlice(_.allocator, chunk);
                _.pos += stop;
            }
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                return try out.toOwnedSlice(_.allocator);
            }
            if (c == '\n' || c == '\r') {
                return error.SyntaxError;
            }
            if (c == '\\') {
                _.pos += 1;
                try _.parseBasicEscape(&out);
                continue;
            }
            return error.SyntaxError;
        }
        return error.UnexpectedEof;
    }

    fn parseBasicString(*_): ([]const u8)! {
        if (_.pos + 2 < _.src.len && _.src[_.pos] == '"' && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
            _.pos += 3;
            return try _.parseMultilineBasicString();
        }
        return try _.parseBasicStringSingleLine();
    }

    /// 多行基础字符串 `"""..."""`（使用 cap:simd Simd.indexOfAny2 块级快扫）
    fn parseMultilineBasicString(*_): ([]const u8)! {
        // 剥离紧随开界符 `"""` 后的首个换行（LF 或 CRLF）
        if (_.pos < _.src.len && _.src[_.pos] == '\r') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
                _.pos += 2;
            } else {
                return error.InvalidControlChar;
            }
        } else if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }

        let out = TomlByteList.empty;
        errdefer out.deinit(_.allocator);
        while (_.pos < _.src.len) {
            const rest = _.src[_.pos..];
            const stop_opt = Simd.indexOfAny2(rest, '"', '\\');
            if (stop_opt == null) {
                return error.UnexpectedEof;
            }
            const stop = stop_opt.?;
            if (stop > 0) {
                const chunk = rest[0..stop];
                try TomlParser.validateNoControlMultiLine(chunk);
                try out.appendSlice(_.allocator, chunk);
                _.pos += stop;
            }
            const c = _.src[_.pos];
            if (c == '"') {
                // 统计连续引号个数：1..2 个为内容，3..5 个为结束（含 0..2 个尾随内容引号），>=6 非法
                let qcnt: usize = 0;
                while (_.pos + qcnt < _.src.len && _.src[_.pos + qcnt] == '"') : (qcnt += 1) {}
                if (qcnt < 3) {
                    try out.appendSlice(_.allocator, _.src[_.pos .. _.pos + qcnt]);
                    _.pos += qcnt;
                    continue;
                }
                if (qcnt > 5) {
                    return error.SyntaxError;
                }
                const extra = qcnt - 3;
                let e: usize = 0;
                while (e < extra) : (e += 1) {
                    try out.append(_.allocator, '"');
                }
                _.pos += qcnt;
                return try out.toOwnedSlice(_.allocator);
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t' || _.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                    // 行尾反斜杠：允许后跟若干空格/制表符 + 至少一个换行，裁剪其后全部空白与换行
                    let scan = _.pos;
                    while (scan < _.src.len && (_.src[scan] == ' ' || _.src[scan] == '\t')) scan += 1;
                    if (scan < _.src.len && (_.src[scan] == '\r' || _.src[scan] == '\n')) {
                        _.pos = scan;
                        while (_.pos < _.src.len) {
                            const wc = _.src[_.pos];
                            if (wc == ' ' || wc == '\t' || wc == '\n') {
                                _.pos += 1;
                            } else if (wc == '\r') {
                                if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
                                    _.pos += 2;
                                } else {
                                    return error.InvalidControlChar;
                                }
                            } else {
                                break;
                            }
                        }
                        continue;
                    }
                    return error.InvalidEscape;
                }
                try _.parseBasicEscape(&out);
                continue;
            }
        }
        return error.UnexpectedEof;
    }

    /// 单行字面量字符串 `'...'`（使用 cap:simd Simd.indexOfAny4 快扫）
    fn parseLiteralStringSingleLine(*_): ([]const u8)! {
        _.pos += 1; // '\''
        const start = _.pos;
        const rest = _.src[_.pos..];
        const stop_opt = Simd.indexOfAny4(rest, '\'', '\n', '\r', 0x7F);
        if (stop_opt == null) {
            return error.UnexpectedEof;
        }
        const stop = stop_opt.?;
        const hit = rest[stop];
        if (hit != '\'') {
            return error.SyntaxError;
        }
        const s = _.src[start .. start + stop];
        try TomlParser.validateNoControlSingleLine(s);
        _.pos = start + stop + 1;
        return s;
    }

    /// 字面量字符串（单行 `'...'` 与多行 `'''...'''`，使用 cap:simd 块级快扫）
    fn parseLiteralString(*_): ([]const u8)! {
        if (_.pos + 2 < _.src.len && _.src[_.pos] == '\'' && _.src[_.pos + 1] == '\'' && _.src[_.pos + 2] == '\'') {
            _.pos += 3;
            if (_.pos < _.src.len && _.src[_.pos] == '\r') {
                if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
                    _.pos += 2;
                } else {
                    return error.InvalidControlChar;
                }
            } else if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            const start = _.pos;
            while (_.pos < _.src.len) {
                const rest = _.src[_.pos..];
                const q_opt = Simd.indexOfByte(rest, '\'');
                if (q_opt == null) {
                    return error.UnexpectedEof;
                }
                _.pos += q_opt.?;
                let qcnt: usize = 0;
                while (_.pos + qcnt < _.src.len && _.src[_.pos + qcnt] == '\'') : (qcnt += 1) {}
                if (qcnt < 3) {
                    _.pos += qcnt;
                    continue;
                }
                if (qcnt > 5) {
                    return error.SyntaxError;
                }
                const extra = qcnt - 3;
                const inner = _.src[start .. _.pos + extra];
                try TomlParser.validateNoControlMultiLine(inner);
                _.pos += qcnt;
                return inner;
            }
            return error.UnexpectedEof;
        }
        return try _.parseLiteralStringSingleLine();
    }

    /// 校验数字段（仅含指定进制字符与合法下划线：下划线两侧必须为合法数字）
    static fn isDigitForBase(c: u8, base: u8): bool {
        if (base == 2) return c == '0' || c == '1';
        if (base == 8) return c >= '0' && c <= '7';
        if (base == 10) return c >= '0' && c <= '9';
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static fn validateDigitsWithUnderscores(s: []const u8, base: u8, forbid_leading_zero: bool): bool {
        if (s.len == 0) return false;
        if (s[0] == '_' || s[s.len - 1] == '_') return false;
        if (forbid_leading_zero && s[0] == '0' && s.len > 1) return false;
        let prev_under = false;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '_') {
                if (prev_under) return false;
                prev_under = true;
            } else {
                if (!TomlParser.isDigitForBase(c, base)) return false;
                prev_under = false;
            }
        }
        return !prev_under;
    }

    /// 判断未引号 token 是否具备日期/时间特征（用于将非法日期归类为 InvalidDateTime）
    static fn looksLikeDateTime(tok: []const u8): bool {
        if (tok.len >= 4 && tok[0] >= '0' && tok[0] <= '9') {
            if (Simd.indexOfByte(tok, ':') != null) return true;
            // 形如 YYYY-MM-...（排除科学计数法 1e-5）
            if (tok.len >= 5 && tok[4] == '-') return true;
            if (tok.len >= 4 && tok[3] == '-' && tok[0] >= '0' && tok[0] <= '9') return true;
        }
        return false;
    }

    /// 严格解析 TOML v1.0.0 数值字面量（整数/浮点/进制/inf/nan）
    static fn parseStrictNumber(tok: []const u8): ConfValue! {
        if (tok.len == 0) return error.InvalidNumber;

        // 1. 特殊浮点常量（大小写严格敏感，严禁下划线）
        if (std.mem.eql(u8, tok, "inf") || std.mem.eql(u8, tok, "+inf")) {
            return ConfValue.makeFloat(std.math.inf(f64));
        }
        if (std.mem.eql(u8, tok, "-inf")) {
            return ConfValue.makeFloat(-1.0 * std.math.inf(f64));
        }
        if (std.mem.eql(u8, tok, "nan") || std.mem.eql(u8, tok, "+nan") || std.mem.eql(u8, tok, "-nan")) {
            return ConfValue.makeFloat(std.math.nan(f64));
        }

        // 2. 非十进制整数（TOML v1.0.0：前缀必须小写 0x/0o/0b，严禁正负号）
        if (tok.len >= 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'o' || tok[1] == 'b')) {
            const base: u8 = if (tok[1] == 'x') 16 else if (tok[1] == 'o') 8 else 2;
            const digits = tok[2..];
            if (!TomlParser.validateDigitsWithUnderscores(digits, base, false)) {
                return error.InvalidNumber;
            }
            const clean = TomlDoc.stripUnderscores(digits);
            const n = std.fmt.parseInt(i64, clean, base) catch return error.InvalidNumber;
            return ConfValue.makeInt(n);
        }
        if (tok.len >= 2 && tok[0] == '0' && (tok[1] == 'X' || tok[1] == 'O' || tok[1] == 'B')) {
            return error.InvalidNumber;
        }

        // 3. 检测是否为浮点数（含 '.' 或 'e' / 'E'）
        let dot_pos: usize? = null;
        let exp_pos: usize? = null;
        let idx: usize = 0;
        while (idx < tok.len) : (idx += 1) {
            const ch = tok[idx];
            if (ch == '.') {
                if (dot_pos != null || exp_pos != null) return error.InvalidNumber;
                dot_pos = idx;
            } else if (ch == 'e' || ch == 'E') {
                if (exp_pos != null) return error.InvalidNumber;
                exp_pos = idx;
            }
        }

        let body = tok;
        if (body[0] == '+' || body[0] == '-') {
            body = body[1..];
            if (body.len == 0) return error.InvalidNumber;
        }

        if (dot_pos != null || exp_pos != null) {
            // 校验浮点三段：int_part [ '.' frac_part ] [ ('e'|'E') ['+'|'-'] exp_part ]
            let sign_off: usize = if (tok[0] == '+' || tok[0] == '-') 1 else 0;
            let int_end = tok.len;
            if (dp := dot_pos) {
                int_end = dp;
            } else if (ep := exp_pos) {
                int_end = ep;
            }
            if (int_end <= sign_off) return error.InvalidNumber;
            if (!TomlParser.validateDigitsWithUnderscores(tok[sign_off..int_end], 10, true)) {
                return error.InvalidNumber;
            }

            if (dp := dot_pos) {
                const frac_end = exp_pos orelse tok.len;
                if (frac_end <= dp + 1) return error.InvalidNumber;
                if (!TomlParser.validateDigitsWithUnderscores(tok[dp + 1 .. frac_end], 10, false)) {
                    return error.InvalidNumber;
                }
            }

            if (ep := exp_pos) {
                let e_start = ep + 1;
                if (e_start >= tok.len) return error.InvalidNumber;
                if (tok[e_start] == '+' || tok[e_start] == '-') {
                    e_start += 1;
                }
                if (e_start >= tok.len) return error.InvalidNumber;
                // TOML v1.0.0：指数部分允许前导零（如 1e06），但下划线规则相同
                if (!TomlParser.validateDigitsWithUnderscores(tok[e_start..], 10, false)) {
                    return error.InvalidNumber;
                }
            }

            const clean = TomlDoc.stripUnderscores(tok);
            let num_s = clean;
            if (clean[0] == '+') num_s = clean[1..];
            const f = std.fmt.parseFloat(f64, num_s) catch return error.InvalidNumber;
            if (std.math.isInf(f) || std.math.isNan(f)) {
                return error.InvalidNumber;
            }
            return ConfValue.makeFloat(f);
        }

        // 4. 十进制整数（严禁前导零，如 01 / +01 / -01 / 00）
        if (!TomlParser.validateDigitsWithUnderscores(body, 10, true)) {
            return error.InvalidNumber;
        }
        const clean = TomlDoc.stripUnderscores(tok);
        let num_s = clean;
        if (clean[0] == '+') num_s = clean[1..];
        const iv = std.fmt.parseInt(i64, num_s, 10) catch return error.InvalidNumber;
        return ConfValue.makeInt(iv);
    }

    fn parseValueNode(*_, depth: usize): usize! {
        if (depth >= TOML_MAX_DEPTH) return error.DepthExceeded;
        _.skipWs();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const c = _.src[_.pos];
        if (c == '"') {
            const s = try _.parseBasicString();
            return try _.allocNode(BuildNode{ kind: NodeKind.Scalar, scalar: ConfValue.makeString(s) });
        }
        if (c == '\'') {
            const s = try _.parseLiteralString();
            return try _.allocNode(BuildNode{ kind: NodeKind.Scalar, scalar: ConfValue.makeString(s) });
        }
        if (c == '[') {
            return try _.parseArrayNode(depth + 1);
        }
        if (c == '{') {
            return try _.parseInlineTableNode(depth + 1);
        }

        // Scan unquoted token (bool, int, float, or RFC 3339 date/time)
        const start = _.pos;
        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if (ch == ',' || ch == ']' || ch == '}' || ch == '#' || ch == '\r' || ch == '\n') {
                break;
            }
            _.pos += 1;
        }
        let tok = std.mem.trim(u8, _.src[start.._.pos], " \t");
        if (tok.len == 0) return error.SyntaxError;

        if (std.mem.eql(u8, tok, "true")) {
            return try _.allocNode(BuildNode{ kind: NodeKind.Scalar, scalar: ConfValue.makeBool(true) });
        }
        if (std.mem.eql(u8, tok, "false")) {
            return try _.allocNode(BuildNode{ kind: NodeKind.Scalar, scalar: ConfValue.makeBool(false) });
        }

        if (dt := ConfDateTime.parse(tok)) {
            if (dt.second > 59) return error.InvalidDateTime;
            return try _.allocNode(BuildNode{ kind: NodeKind.Scalar, scalar: ConfValue.makeDateTime(dt) });
        }
        if (TomlParser.looksLikeDateTime(tok)) {
            return error.InvalidDateTime;
        }

        const num_val = try TomlParser.parseStrictNumber(tok);
        return try _.allocNode(BuildNode{ kind: NodeKind.Scalar, scalar: num_val });
    }

    fn parseArrayNode(*_, depth: usize): usize! {
        if (depth >= TOML_MAX_DEPTH) return error.DepthExceeded;
        _.pos += 1; // '['
        const arr_id = try _.allocNode(BuildNode{ kind: NodeKind.ArrayStatic });
        while (true) {
            try _.skipWsAndComments();
            if (_.pos >= _.src.len) return error.UnexpectedEof;
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            const elem_id = try _.parseValueNode(depth + 1);
            try _.nodes.items[arr_id].items.append(_.allocator, elem_id);
            try _.skipWsAndComments();
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.pos < _.src.len && _.src[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            return error.SyntaxError;
        }
        return arr_id;
    }

    /// TOML v1.0.0 Inline Table：单行、禁止换行、禁止尾随逗号、创建后永久冻结 (TableInlineFrozen)
    fn parseInlineTableNode(*_, depth: usize): usize! {
        if (depth >= TOML_MAX_DEPTH) return error.DepthExceeded;
        _.pos += 1; // '{'
        const tbl_id = try _.allocNode(BuildNode{ kind: NodeKind.TableInlineFrozen, scope_id: _.scope_counter });

        _.skipWs();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        if (_.src[_.pos] == '}') {
            _.pos += 1;
            return tbl_id;
        }

        let inline_scope = _.scope_counter + 1000000 + depth;
        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len) return error.UnexpectedEof;
            if (_.src[_.pos] == '\n' || _.src[_.pos] == '\r') return error.SyntaxError;
            if (_.src[_.pos] == '}') {
                // TOML v1.0.0 禁止 Inline Table 尾随逗号 `{ a = 1, }`
                return error.SyntaxError;
            }
            let segs: [32][]const u8 = undefined;
            const seg_count = try _.parseKeyPath(&segs);
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '=') return error.SyntaxError;
            _.pos += 1;
            const val_id = try _.parseValueNode(depth + 1);
            try _.insertDottedKey(tbl_id, segs[0..seg_count], val_id, inline_scope);
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.pos < _.src.len && _.src[_.pos] == '}') {
                _.pos += 1;
                break;
            }
            return error.SyntaxError;
        }
        return tbl_id;
    }

    fn findFieldNodeId(*_, tbl_id: usize, key: []const u8): usize? {
        const fields = _.nodes.items[tbl_id].fields.items;
        let i: usize = 0;
        while (i < fields.len) : (i += 1) {
            if (std.mem.eql(u8, fields[i].key, key)) {
                return fields[i].node_id;
            }
        }
        return null;
    }

    fn addFieldId(*_, tbl_id: usize, key: []const u8, child_id: usize): ! {
        try _.nodes.items[tbl_id].fields.append(_.allocator, BuildField{ key: key, node_id: child_id });
    }

    /// 点分键赋值（如 `a.b.c = val`）：严格执行重复键拒绝、Inline Table 冻结与作用域限制
    fn insertDottedKey(*_, target_id: usize, segs: []const []const u8, val_id: usize, cur_scope: usize): ! {
        if (segs.len == 0) {
            return error.SyntaxError;
        }
        let cur_id = target_id;
        let i: usize = 0;
        while (i + 1 < segs.len) : (i += 1) {
            const seg = segs[i];
            if (existing_id := _.findFieldNodeId(cur_id, seg)) {
                const k = _.nodes.items[existing_id].kind;
                if (k == NodeKind.TableInlineFrozen) {
                    return error.FrozenTable;
                }
                if (k == NodeKind.TableExplicit) {
                    return error.TableRedefined;
                }
                if (k == NodeKind.TableDotted) {
                    if (_.nodes.items[existing_id].scope_id != cur_scope) {
                        return error.TableRedefined;
                    }
                    cur_id = existing_id;
                } else if (k == NodeKind.TableImplicit) {
                    cur_id = existing_id;
                } else {
                    return error.TableRedefined;
                }
            } else {
                const new_id = try _.allocNode(BuildNode{
                    kind: NodeKind.TableDotted,
                    scope_id: cur_scope,
                });
                try _.addFieldId(cur_id, seg, new_id);
                cur_id = new_id;
            }
        }
        const leaf_key = segs[segs.len - 1];
        if (_.findFieldNodeId(cur_id, leaf_key) != null) {
            return error.DuplicateKey;
        }
        try _.addFieldId(cur_id, leaf_key, val_id);
    }

    /// 导航表头中间路径（`[a.b.c]` 中的 `a`, `b`）
    fn navigateTableHeaderParent(*_, parent_id: usize, seg: []const u8): usize! {
        if (existing_id := _.findFieldNodeId(parent_id, seg)) {
            const k = _.nodes.items[existing_id].kind;
            if (k == NodeKind.TableInlineFrozen) {
                return error.FrozenTable;
            }
            if (k == NodeKind.ArrayOfTables) {
                const len = _.nodes.items[existing_id].items.items.len;
                if (len == 0) return error.TableRedefined;
                return _.nodes.items[existing_id].items.items[len - 1];
            }
            if (BuildNode.isTableKind(k)) {
                return existing_id;
            }
            return error.TableRedefined;
        }
        const new_id = try _.allocNode(BuildNode{ kind: NodeKind.TableImplicit });
        try _.addFieldId(parent_id, seg, new_id);
        return new_id;
    }

    /// 定义显式表 `[...leaf]`
    fn defineExplicitTable(*_, parent_id: usize, leaf: []const u8): usize! {
        if (existing_id := _.findFieldNodeId(parent_id, leaf)) {
            const k = _.nodes.items[existing_id].kind;
            if (k == NodeKind.TableImplicit) {
                _.nodes.items[existing_id].kind = NodeKind.TableExplicit;
                return existing_id;
            }
            if (k == NodeKind.TableInlineFrozen) {
                return error.FrozenTable;
            }
            return error.TableRedefined;
        }
        const new_id = try _.allocNode(BuildNode{ kind: NodeKind.TableExplicit });
        try _.addFieldId(parent_id, leaf, new_id);
        return new_id;
    }

    /// 追加表数组元素 `[[...leaf]]`
    fn appendTableArrayNode(*_, parent_id: usize, leaf: []const u8): usize! {
        if (existing_id := _.findFieldNodeId(parent_id, leaf)) {
            if (_.nodes.items[existing_id].kind != NodeKind.ArrayOfTables) {
                return error.TableRedefined;
            }
            const elem_id = try _.allocNode(BuildNode{ kind: NodeKind.TableExplicit });
            try _.nodes.items[existing_id].items.append(_.allocator, elem_id);
            return elem_id;
        }
        const arr_id = try _.allocNode(BuildNode{ kind: NodeKind.ArrayOfTables });
        const elem_id = try _.allocNode(BuildNode{ kind: NodeKind.TableExplicit });
        try _.nodes.items[arr_id].items.append(_.allocator, elem_id);
        try _.addFieldId(parent_id, leaf, arr_id);
        return elem_id;
    }

    fn runParser(*_): ConfValue! {
        const root_id = try _.allocNode(BuildNode{ kind: NodeKind.TableExplicit, scope_id: 1 });
        let active_table_id: usize = root_id;
        let cur_scope: usize = 1;

        while (true) {
            try _.skipWsAndComments();
            if (_.pos >= _.src.len) break;

            if (_.src[_.pos] == '[') {
                _.pos += 1;
                const is_table_array = (_.pos < _.src.len && _.src[_.pos] == '[');
                if (is_table_array) _.pos += 1;

                let segs: [32][]const u8 = undefined;
                const seg_count = try _.parseKeyPath(&segs);
                _.skipWs();
                if (_.pos >= _.src.len || _.src[_.pos] != ']') return error.SyntaxError;
                _.pos += 1;
                if (is_table_array) {
                    if (_.pos >= _.src.len || _.src[_.pos] != ']') return error.SyntaxError;
                    _.pos += 1;
                }
                try _.expectLineEndOrEof();

                let cur_id: usize = root_id;
                let i: usize = 0;
                while (i + 1 < seg_count) : (i += 1) {
                    cur_id = try _.navigateTableHeaderParent(cur_id, segs[i]);
                }
                _.scope_counter += 1;
                cur_scope = _.scope_counter;
                if (is_table_array) {
                    active_table_id = try _.appendTableArrayNode(cur_id, segs[seg_count - 1]);
                } else {
                    active_table_id = try _.defineExplicitTable(cur_id, segs[seg_count - 1]);
                }
                continue;
            }

            // Key = Value
            let segs: [32][]const u8 = undefined;
            const seg_count = try _.parseKeyPath(&segs);
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '=') return error.SyntaxError;
            _.pos += 1;
            const val_id = try _.parseValueNode(0);
            try _.expectLineEndOrEof();
            try _.insertDottedKey(active_table_id, segs[0..seg_count], val_id, cur_scope);
        }

        return try _.nodeToConfValue(root_id);
    }

    // ========================================================================
    // Bidirectional TOML 1.0 Serialization
    // ========================================================================

    pub static fn stringify(allocator: std.mem.Allocator, val: ConfValue): ([]const u8)! {
        let out = TomlByteList.empty;
        if (val.tag != ConfTag.Object) return error.SyntaxError;
        try TomlParser.writeTableBody(&out, allocator, "", val);
        return try out.toOwnedSlice(allocator);
    }

    static fn isBareKey(s: []const u8): bool {
        if (s.len == 0) return false;
        for (c of s) {
            if (!TomlParser.isBareKeyChar(c)) return false;
        }
        return true;
    }

    static fn writeKey(out: *TomlByteList, allocator: std.mem.Allocator, k: []const u8): ! {
        if (TomlParser.isBareKey(k)) {
            try out.appendSlice(allocator, k);
            return;
        }
        try TomlParser.writeQuotedString(out, allocator, k);
    }

    static fn writeQuotedString(out: *TomlByteList, allocator: std.mem.Allocator, s: []const u8): ! {
        const hex_digits = "0123456789ABCDEF";
        try out.append(allocator, '"');
        for (c of s) {
            switch (c) {
                0x08 => try out.appendSlice(allocator, "\\b"),
                0x0C => try out.appendSlice(allocator, "\\f"),
                '"' => try out.appendSlice(allocator, "\\\""),
                '\\' => try out.appendSlice(allocator, "\\\\"),
                '\n' => try out.appendSlice(allocator, "\\n"),
                '\r' => try out.appendSlice(allocator, "\\r"),
                '\t' => try out.appendSlice(allocator, "\\t"),
                else => {
                    if (c < 0x20 || c == 0x7F) {
                        const esc: [6]u8 = [
                            '\\',
                            'u',
                            '0',
                            '0',
                            hex_digits[(c >> 4) & 0xF],
                            hex_digits[c & 0xF],
                        ];
                        try out.appendSlice(allocator, esc[0..]);
                    } else {
                        try out.append(allocator, c);
                    }
                },
            }
        }
        try out.append(allocator, '"');
    }

    static fn isTableArray(v: ConfValue): bool {
        return v.tag == ConfTag.Array && v.arr_val.len > 0 && v.arr_val[0].tag == ConfTag.Object;
    }

    static fn writeInlineValue(out: *TomlByteList, allocator: std.mem.Allocator, val: ConfValue): ! {
        switch (val.tag) {
            .Null => try out.appendSlice(allocator, "\"\""),
            .Bool => try out.appendSlice(allocator, if (val.bool_val) "true" else "false"),
            .Int => {
                const s = try std.fmt.allocPrint(allocator, "{d}", .{val.int_val});
                defer allocator.free(s);
                try out.appendSlice(allocator, s);
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    try out.appendSlice(allocator, "nan");
                } else if (std.math.isInf(val.float_val)) {
                    try out.appendSlice(allocator, if (val.float_val < 0) "-inf" else "inf");
                } else {
                    const s = try std.fmt.allocPrint(allocator, "{d}", .{val.float_val});
                    defer allocator.free(s);
                    try out.appendSlice(allocator, s);
                }
            },
            .String => try TomlParser.writeQuotedString(out, allocator, val.str_val),
            .DateTime => try out.appendSlice(allocator, val.dt_val.raw),
            .Array => {
                try out.appendSlice(allocator, "[ ");
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) try out.appendSlice(allocator, ", ");
                    try TomlParser.writeInlineValue(out, allocator, val.arr_val[i]);
                }
                try out.appendSlice(allocator, " ]");
            },
            .Object => {
                try out.appendSlice(allocator, "{ ");
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    if (i > 0) try out.appendSlice(allocator, ", ");
                    try TomlParser.writeKey(out, allocator, val.obj_val[i].key);
                    try out.appendSlice(allocator, " = ");
                    try TomlParser.writeInlineValue(out, allocator, val.obj_val[i].value);
                }
                try out.appendSlice(allocator, " }");
            },
        }
    }

    static fn writeTableBody(out: *TomlByteList, allocator: std.mem.Allocator, prefix: []const u8, obj: ConfValue): ! {
        // 1. Scalar / regular array / empty-object fields
        for (f of obj.obj_val) {
            if (f.value.tag == ConfTag.Object || TomlParser.isTableArray(f.value)) continue;
            try TomlParser.writeKey(out, allocator, f.key);
            try out.appendSlice(allocator, " = ");
            try TomlParser.writeInlineValue(out, allocator, f.value);
            try out.append(allocator, '\n');
        }
        // 2. Sub-tables
        for (f of obj.obj_val) {
            if (f.value.tag != ConfTag.Object) continue;
            let key_buf = TomlByteList.empty;
            defer key_buf.deinit(allocator);
            try TomlParser.writeKey(&key_buf, allocator, f.key);
            const full_name = if (prefix.len == 0)
                try allocator.dupe(u8, key_buf.items)
            else
                try std.fmt.allocPrint(allocator, "{s}.{s}", .{ prefix, key_buf.items });
            defer allocator.free(full_name);

            if (out.items.len > 0) try out.append(allocator, '\n');
            try out.append(allocator, '[');
            try out.appendSlice(allocator, full_name);
            try out.appendSlice(allocator, "]\n");
            try TomlParser.writeTableBody(out, allocator, full_name, f.value);
        }
        // 3. Array of tables [[...]]
        for (f of obj.obj_val) {
            if (!TomlParser.isTableArray(f.value)) continue;
            let key_buf = TomlByteList.empty;
            defer key_buf.deinit(allocator);
            try TomlParser.writeKey(&key_buf, allocator, f.key);
            const full_name = if (prefix.len == 0)
                try allocator.dupe(u8, key_buf.items)
            else
                try std.fmt.allocPrint(allocator, "{s}.{s}", .{ prefix, key_buf.items });
            defer allocator.free(full_name);

            for (elem of f.value.arr_val) {
                if (out.items.len > 0) try out.append(allocator, '\n');
                try out.appendSlice(allocator, "[[");
                try out.appendSlice(allocator, full_name);
                try out.appendSlice(allocator, "]]\n");
                try TomlParser.writeTableBody(out, allocator, full_name, elem);
            }
        }
    }
}

pub struct TomlDoc {
    pub entries: [512]TomlEntry = undefined;
    pub count: usize = 0;

    pub static fn parse(content: []const u8): TomlDoc {
        let doc = TomlDoc {
            entries: undefined,
            count: 0,
        };
        doc.parseContent(content);
        return doc;
    }

    pub static fn parseTree(allocator: std.mem.Allocator, content: []const u8): ConfValue! {
        return try TomlParser.parse(allocator, content);
    }

    pub fn get(table: []const u8, key: []const u8): ([]const u8)? {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.entries[i].table, table) &&
                std.mem.eql(u8, _.entries[i].key, key))
            {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn getRoot(key: []const u8): ([]const u8)? {
        return _.get("", key);
    }

    pub fn getOr(table: []const u8, key: []const u8, default_val: []const u8): []const u8 {
        if (v := _.get(table, key)) {
            return v;
        }
        return default_val;
    }

    pub fn getDateTime(table: []const u8, key: []const u8): ConfDateTime? {
        if (v := _.get(table, key)) {
            return ConfDateTime.parse(v);
        }
        return null;
    }

    pub fn getInt(table: []const u8, key: []const u8, default_val: i64): i64 {
        if (v := _.get(table, key)) {
            const clean = TomlDoc.stripUnderscores(v);
            if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 16) catch null) {
                    return val;
                }
            } else if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'o' || clean[1] == 'O')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 8) catch null) {
                    return val;
                }
            } else if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'b' || clean[1] == 'B')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 2) catch null) {
                    return val;
                }
            } else {
                let to_parse = clean;
                if (clean.len > 0 && clean[0] == '+') {
                    to_parse = clean[1..];
                }
                if (val := std.fmt.parseInt(i64, to_parse, 10) catch null) {
                    return val;
                }
            }
        }
        return default_val;
    }

    pub fn getFloat(table: []const u8, key: []const u8, default_val: f64): f64 {
        if (v := _.get(table, key)) {
            const clean = TomlDoc.stripUnderscores(v);
            if (std.mem.eql(u8, clean, "inf") || std.mem.eql(u8, clean, "+inf")) {
                return std.math.inf(f64);
            }
            if (std.mem.eql(u8, clean, "-inf")) {
                return -1.0 * std.math.inf(f64);
            }
            if (std.mem.eql(u8, clean, "nan") || std.mem.eql(u8, clean, "+nan") || std.mem.eql(u8, clean, "-nan")) {
                return std.math.nan(f64);
            }
            let to_parse = clean;
            if (clean.len > 0 && clean[0] == '+') {
                to_parse = clean[1..];
            }
            if (val := std.fmt.parseFloat(f64, to_parse) catch null) {
                return val;
            }
        }
        return default_val;
    }

    pub fn getBool(table: []const u8, key: []const u8, default_val: bool): bool {
        if (v := _.get(table, key)) {
            if (std.mem.eql(u8, v, "true")) {
                return true;
            }
            if (std.mem.eql(u8, v, "false")) {
                return false;
            }
        }
        return default_val;
    }

    pub fn getArray(table: []const u8, key: []const u8): [][]const u8 {
        if (v := _.get(table, key)) {
            return TomlDoc.parseArrayElements(v);
        }
        let empty_buf = std.heap.page_allocator.alloc(string, 0) catch unreachable;
        return empty_buf;
    }

    pub fn hasTable(table: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn hasKey(table: []const u8, key: []const u8): bool {
        return _.get(table, key) != null;
    }

    pub fn set(*_, table: []const u8, key: []const u8, value: []const u8) {
        _.put(table, key, value);
    }

    pub fn setRoot(*_, key: []const u8, value: []const u8) {
        _.put("", key, value);
    }

    pub fn remove(*_, table: []const u8, key: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) && std.mem.eql(u8, _.entries[i].key, key)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j += 1;
                }
                _.count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn removeTable(*_, table: []const u8): bool {
        let removed = false;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j += 1;
                }
                _.count -= 1;
                removed = true;
            } else {
                i += 1;
            }
        }
        return removed;
    }

    pub fn getTables(*_): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            const tbl = _.entries[i].table;
            if (tbl.len > 0) {
                let found = false;
                let k: usize = 0;
                while (k < n) {
                    if (std.mem.eql(u8, buf[k], tbl)) {
                        found = true;
                        break;
                    }
                    k += 1;
                }
                if (!found && n < 64) {
                    buf[n] = tbl;
                    n += 1;
                }
            }
            i += 1;
        }
        return buf[0..n];
    }

    pub fn getKeys(*_, table: []const u8): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 128) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) && n < 128) {
                buf[n] = _.entries[i].key;
                n += 1;
            }
            i += 1;
        }
        return buf[0..n];
    }

    static fn formatScalarToString(val: ConfValue): []const u8 {
        const alloc = std.heap.page_allocator;
        return switch (val.tag) {
            .Null => "",
            .Bool => if (val.bool_val) "true" else "false",
            .Int => std.fmt.allocPrint(alloc, "{d}", .{val.int_val}) catch "",
            .Float => std.fmt.allocPrint(alloc, "{d}", .{val.float_val}) catch "",
            .String => val.str_val,
            .DateTime => val.dt_val.raw,
            .Array => {
                let out = TomlByteList.empty;
                out.append(alloc, '[') catch return "[]";
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) out.appendSlice(alloc, ", ") catch {};
                    const item = val.arr_val[i];
                    if (item.tag == ConfTag.String) {
                        out.append(alloc, '"') catch {};
                        out.appendSlice(alloc, item.str_val) catch {};
                        out.append(alloc, '"') catch {};
                    } else {
                        out.appendSlice(alloc, TomlDoc.formatScalarToString(item)) catch {};
                    }
                }
                out.append(alloc, ']') catch {};
                return out.toOwnedSlice(alloc) catch "[]";
            },
            .Object => "{...}",
        };
    }

    fn flattenTree(*_, prefix: []const u8, val: ConfValue) {
        if (val.tag != ConfTag.Object) return;
        const alloc = std.heap.page_allocator;
        for (f of val.obj_val) {
            if (f.value.tag == ConfTag.Object) {
                let sub = f.key;
                if (prefix.len > 0) {
                    sub = std.fmt.allocPrint(alloc, "{s}.{s}", .{ prefix, f.key }) catch f.key;
                }
                _.flattenTree(sub, f.value);
            } else if (f.value.tag == ConfTag.Array && f.value.arr_val.len > 0 && f.value.arr_val[0].tag == ConfTag.Object) {
                let arr_tbl = f.key;
                if (prefix.len > 0) {
                    arr_tbl = std.fmt.allocPrint(alloc, "{s}.{s}", .{ prefix, f.key }) catch f.key;
                }
                let idx: usize = 0;
                while (idx < f.value.arr_val.len) : (idx += 1) {
                    const indexed_tbl = std.fmt.allocPrint(alloc, "{s}.{d}", .{ arr_tbl, idx }) catch arr_tbl;
                    _.flattenTree(indexed_tbl, f.value.arr_val[idx]);
                    _.flattenTree(arr_tbl, f.value.arr_val[idx]);
                }
            } else {
                _.put(prefix, f.key, TomlDoc.formatScalarToString(f.value));
            }
        }
    }

    fn parseContent(*_, content: []const u8) {
        const tree = TomlParser.parse(std.heap.page_allocator, content) catch {
            _.parseFallback(content);
            return;
        };
        _.flattenTree("", tree);
    }

    fn parseFallback(*_, content: []const u8) {
        let current_table: []const u8 = "";
        let line_iter = std.mem.splitScalar(u8, content, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == '#') {
                continue;
            }

            if (line.len >= 4 && line[0] == '[' && line[1] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 2 && line[close_idx - 1] == ']') {
                        current_table = std.mem.trim(u8, line[2 .. close_idx - 1], " \t");
                    }
                }
                continue;
            }

            if (line[0] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 1) {
                        current_table = std.mem.trim(u8, line[1..close_idx], " \t");
                    }
                }
                continue;
            }

            if (eq_idx := std.mem.indexOfScalar(u8, line, '=')) {
                const raw_key = std.mem.trim(u8, line[0..eq_idx], " \t\"'");
                if (raw_key.len == 0) {
                    continue;
                }

                let target_table = current_table;
                let target_key = raw_key;

                if (dot_idx := std.mem.lastIndexOfScalar(u8, raw_key, '.')) {
                    if (dot_idx > 0 && dot_idx + 1 < raw_key.len) {
                        const prefix = raw_key[0..dot_idx];
                        target_key = raw_key[dot_idx + 1 ..];
                        if (current_table.len > 0) {
                            let comb = std.heap.page_allocator.alloc(u8, current_table.len + 1 + prefix.len) catch unreachable;
                            @memcpy(comb[0..current_table.len], current_table);
                            comb[current_table.len] = '.';
                            @memcpy(comb[current_table.len + 1 ..], prefix);
                            target_table = comb;
                        } else {
                            target_table = prefix;
                        }
                    }
                }

                const raw_val = std.mem.trim(u8, line[eq_idx + 1 ..], " \t");
                const val = TomlDoc.cleanTomlValue(raw_val);
                _.put(target_table, target_key, val);
            }
        }
    }

    static fn cleanTomlValue(raw: []const u8): []const u8 {
        if (raw.len == 0) {
            return "";
        }
        if (raw.len >= 6 && std.mem.startsWith(u8, raw, "\"\"\"") && std.mem.endsWith(u8, raw, "\"\"\"")) {
            return TomlDoc.unescapeToml(raw[3 .. raw.len - 3]);
        }
        if (raw.len >= 6 && std.mem.startsWith(u8, raw, "'''") && std.mem.endsWith(u8, raw, "'''")) {
            return raw[3 .. raw.len - 3];
        }
        if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
            return TomlDoc.unescapeToml(raw[1 .. raw.len - 1]);
        }
        if (raw.len >= 2 && raw[0] == '\'' && raw[raw.len - 1] == '\'') {
            return raw[1 .. raw.len - 1];
        }
        if ((raw[0] == '[' && raw[raw.len - 1] == ']') ||
            (raw[0] == '{' && raw[raw.len - 1] == '}'))
        {
            return raw;
        }
        let end = raw.len;
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '#' && i > 0 && (raw[i - 1] == ' ' || raw[i - 1] == '\t')) {
                end = i - 1;
                break;
            }
            i += 1;
        }
        return std.mem.trim(u8, raw[0..end], " \t\r");
    }

    static fn unescapeToml(s: []const u8): []const u8 {
        if (Simd.indexOfByte(s, '\\') == null) {
            return s;
        }
        let buf = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                const next_ch = s[i + 1];
                if (next_ch == 'n') {
                    buf[out_idx] = '\n';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == 'r') {
                    buf[out_idx] = '\r';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == 't') {
                    buf[out_idx] = '\t';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == '\\') {
                    buf[out_idx] = '\\';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == '"') {
                    buf[out_idx] = '"';
                    out_idx += 1;
                    i += 2;
                    continue;
                }
            }
            buf[out_idx] = s[i];
            out_idx += 1;
            i += 1;
        }
        return buf[0..out_idx];
    }

    pub static fn stripUnderscores(s: []const u8): []const u8 {
        if (Simd.indexOfByte(s, '_') == null) {
            return s;
        }
        let buf = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] != '_') {
                buf[out_idx] = s[i];
                out_idx += 1;
            }
            i += 1;
        }
        return buf[0..out_idx];
    }

    static fn parseArrayElements(raw: []const u8): [][]const u8 {
        const trimmed = std.mem.trim(u8, raw, " \t\r\n");
        if (trimmed.len < 2 || trimmed[0] != '[' || trimmed[trimmed.len - 1] != ']') {
            let empty = std.heap.page_allocator.alloc(string, 0) catch unreachable;
            return empty;
        }
        const inner = trimmed[1 .. trimmed.len - 1];
        let buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let count: usize = 0;

        let in_quotes = false;
        let quote_char: u8 = 0;
        let start: usize = 0;
        let i: usize = 0;

        while (i < inner.len) {
            const ch = inner[i];
            if (in_quotes) {
                if (ch == quote_char && (i == 0 || inner[i - 1] != '\\')) {
                    in_quotes = false;
                }
            } else {
                if (ch == '"' || ch == '\'') {
                    in_quotes = true;
                    quote_char = ch;
                } else if (ch == ',') {
                    const item = std.mem.trim(u8, inner[start..i], " \t\r\n");
                    if (item.len > 0 && count < 64) {
                        buf[count] = TomlDoc.cleanTomlValue(item);
                        count += 1;
                    }
                    start = i + 1;
                }
            }
            i += 1;
        }

        const last_item = std.mem.trim(u8, inner[start..], " \t\r\n");
        if (last_item.len > 0 && count < 64) {
            buf[count] = TomlDoc.cleanTomlValue(last_item);
            count += 1;
        }

        return buf[0..count];
    }

    fn put(*_, table: []const u8, key: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) &&
                std.mem.eql(u8, _.entries[i].key, key))
            {
                _.entries[i].value = value;
                return;
            }
            i += 1;
        }
        if (_.count < _.entries.len) {
            _.entries[_.count] = TomlEntry {
                table: table,
                key: key,
                value: value,
            };
            _.count += 1;
        }
    }

    pub fn toString(): []const u8 {
        let buf = std.heap.page_allocator.alloc(u8, 32768) catch unreachable;
        let len: usize = 0;

        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].table.len == 0) {
                const k = _.entries[i].key;
                const v = _.entries[i].value;
                let idx: usize = 0;
                while (idx < k.len) { buf[len] = k[idx]; len += 1; idx += 1; }
                buf[len] = ' '; len += 1;
                buf[len] = '='; len += 1;
                buf[len] = ' '; len += 1;
                idx = 0;
                while (idx < v.len) { buf[len] = v[idx]; len += 1; idx += 1; }
                buf[len] = '\n'; len += 1;
            }
            i += 1;
        }

        let tbl_buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let tbl_count: usize = 0;
        i = 0;
        while (i < _.count) {
            const tbl = _.entries[i].table;
            if (tbl.len > 0) {
                let found = false;
                let k: usize = 0;
                while (k < tbl_count) {
                    if (std.mem.eql(u8, tbl_buf[k], tbl)) {
                        found = true;
                        break;
                    }
                    k += 1;
                }
                if (!found && tbl_count < 64) {
                    tbl_buf[tbl_count] = tbl;
                    tbl_count += 1;
                }
            }
            i += 1;
        }

        let t: usize = 0;
        while (t < tbl_count) {
            const tbl_name = tbl_buf[t];
            if (len > 0) {
                buf[len] = '\n'; len += 1;
            }
            buf[len] = '['; len += 1;
            let idx: usize = 0;
            while (idx < tbl_name.len) { buf[len] = tbl_name[idx]; len += 1; idx += 1; }
            buf[len] = ']'; len += 1;
            buf[len] = '\n'; len += 1;

            i = 0;
            while (i < _.count) {
                if (std.mem.eql(u8, _.entries[i].table, tbl_name)) {
                    const k = _.entries[i].key;
                    const v = _.entries[i].value;
                    idx = 0;
                    while (idx < k.len) { buf[len] = k[idx]; len += 1; idx += 1; }
                    buf[len] = ' '; len += 1;
                    buf[len] = '='; len += 1;
                    buf[len] = ' '; len += 1;
                    idx = 0;
                    while (idx < v.len) { buf[len] = v[idx]; len += 1; idx += 1; }
                    buf[len] = '\n'; len += 1;
                }
                i += 1;
            }
            t += 1;
        }

        return buf[0..len];
    }

    pub fn encode(): []const u8 {
        return _.toString();
    }
}

pub type toml = TomlDoc;
pub type TOML = TomlDoc;
