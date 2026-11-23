// Reference-hardened against Ethiraric/yaml-rust2 (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/interchange/yaml.cp | Synced: 2026-11-22T19:03:00Z
// lib/conf/yamlParser.cp
// Industrial-grade YAML 1.2 Parser & Bidirectional Serializer:
// - Multi-document streams (`---` / `...`, `%YAML`/`%TAG`)
// - Anchors & Aliases (`&anchor` / `*alias`) with Circular Alias & Expansion Budget protection
// - Merge Keys (`<<: *base` and `<<: [*a, *b]`) with bounded node budget
// - Block Scalars (`|`, `|-`, `|+`, `>`, `>-`, `>+`, explicit indent indicator `1..9`)
// - Flow Sequences & Mappings (`[a, {b: [1, 2]}]`) with max depth guard (`YAML_MAX_DEPTH = 64`)
// - Explicit Type Tags (`!!str`, `!!int`, `!!float`, `!!bool`, `!!null`, `!!seq`, `!!map`)
// - Single & Double Quoted Escapes (`\n`/`\t`/`\xHH`/`\uXXXX`/`\UXXXXXXXX`) + YAML 1.2 Core Schema
// - Hot-path `cap:simd` acceleration for line-end/comment scan, indent counting, and quoted string scan
// - Recursive ConfValue AST & Bidirectional YAML Serialization

const std = @import("std");
import { Simd } from "cap:simd";
import { ConfTag, ConfField, ConfValue } from "./value";

pub const YAML_MAX_DEPTH: usize = 64;
pub const YAML_MAX_ALIAS_EXPANSIONS: usize = 256;
pub const YAML_MAX_EXPANDED_NODES: usize = 16384;

pub error YamlError {
    OutOfMemory,
    UnexpectedEof,
    SyntaxError,
    UnknownAlias,
    InvalidEscape,
    InvalidTag,
    InvalidIndentation,
    DepthExceeded,
    CircularAlias,
    AliasExpansionLimitExceeded,
    TabIndentationForbidden,
}

pub struct YamlAnchor {
    pub name: []const u8 = "",
    pub value: ConfValue = ConfValue{},
}

pub struct YamlStream {
    pub docs: []ConfValue = [],

    pub fn count(): usize {
        return _.docs.len;
    }

    pub fn getDoc(index: usize): ConfValue? {
        if (index >= _.docs.len) return null;
        return _.docs[index];
    }
}

enum YamlTagOverride: u8 {
    None = 0,
    Str = 1,
    Int = 2,
    Float = 3,
    Bool = 4,
    Null = 5,
    Seq = 6,
    Map = 7,
}

struct NodeProps {
    anchor: ([]const u8)? = null;
    tag: YamlTagOverride = YamlTagOverride.None;
}

type YamlValueList = std.ArrayListUnmanaged(ConfValue);
type YamlFieldList = std.ArrayListUnmanaged(ConfField);
type YamlByteList = std.ArrayListUnmanaged(u8);
type YamlStrList = std.ArrayListUnmanaged(string);
type YamlAnchorList = std.ArrayListUnmanaged(YamlAnchor);

pub struct YamlParser {
    src: []const u8 = "",
    pos: usize = 0,
    allocator: std.mem.Allocator = undefined,
    anchors: YamlAnchorList = YamlAnchorList.empty,
    active_anchors: YamlStrList = YamlStrList.empty,
    depth: usize = 0,
    alias_expansions: usize = 0,
    expanded_nodes: usize = 0,

    pub static fn init(allocator: std.mem.Allocator, src: []const u8): YamlParser {
        return YamlParser{
            src: src,
            pos: 0,
            allocator: allocator,
            anchors: YamlAnchorList.empty,
            active_anchors: YamlStrList.empty,
            depth: 0,
            alias_expansions: 0,
            expanded_nodes: 0,
        };
    }

    pub fn deinit(*_) {
        _.anchors.deinit(_.allocator);
        _.active_anchors.deinit(_.allocator);
    }

    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): ConfValue! {
        let stream = try YamlParser.parseStream(allocator, src);
        if (stream.docs.len == 0) return ConfValue.makeNull();
        return stream.docs[0];
    }

    pub static fn parseStream(allocator: std.mem.Allocator, src: []const u8): YamlStream! {
        let p = YamlParser.init(allocator, src);
        defer p.deinit();
        return try p.runStream();
    }

    // ========================================================================
    // Hot-Path cap:simd Scanners & Scalar Reference Counterparts
    // ========================================================================

    /// SIMD-accelerated scan for next line break (`\n` or `\r`); returns `slice.len` if none.
    pub static fn scanLineEndSimd(slice: []const u8): usize {
        if (idx := Simd.indexOfAny2(slice, '\n', '\r')) {
            return idx;
        }
        return slice.len;
    }

    /// Scalar reference for `scanLineEndSimd`.
    pub static fn scanLineEndScalar(slice: []const u8): usize {
        let i: usize = 0;
        while (i < slice.len) : (i += 1) {
            if (slice[i] == '\n' || slice[i] == '\r') return i;
        }
        return slice.len;
    }

    /// SIMD-accelerated count of leading ASCII space (`' '`) indentation bytes.
    pub static fn countIndentSpacesSimd(slice: []const u8): usize {
        if (slice.len < 16) {
            return YamlParser.countIndentSpacesScalar(slice);
        }
        const ws_len = Simd.skipWhitespace(slice);
        if (ws_len == 0) return 0;
        const ws_slice = slice[0..ws_len];
        if (non_sp := Simd.indexOfAny4(ws_slice, '\t', '\r', '\n', '\t')) {
            return non_sp;
        }
        return ws_len;
    }

    /// Scalar reference for `countIndentSpacesSimd`.
    pub static fn countIndentSpacesScalar(slice: []const u8): usize {
        let i: usize = 0;
        while (i < slice.len && slice[i] == ' ') : (i += 1) {}
        return i;
    }

    /// SIMD-accelerated scan for next double-quoted stop byte (`"`, `\`, `\n`, `\r`).
    pub static fn scanDoubleQuotedStopSimd(slice: []const u8): usize? {
        return Simd.indexOfAny4(slice, '"', '\\', '\n', '\r');
    }

    /// Scalar reference for `scanDoubleQuotedStopSimd`.
    pub static fn scanDoubleQuotedStopScalar(slice: []const u8): usize? {
        let i: usize = 0;
        while (i < slice.len) : (i += 1) {
            const c = slice[i];
            if (c == '"' || c == '\\' || c == '\n' || c == '\r') return i;
        }
        return null;
    }

    /// SIMD-accelerated scan for next single-quoted stop byte (`'`, `\n`, `\r`).
    pub static fn scanSingleQuotedStopSimd(slice: []const u8): usize? {
        return Simd.indexOfAny4(slice, '\'', '\n', '\r', '\'');
    }

    /// Scalar reference for `scanSingleQuotedStopSimd`.
    pub static fn scanSingleQuotedStopScalar(slice: []const u8): usize? {
        let i: usize = 0;
        while (i < slice.len) : (i += 1) {
            const c = slice[i];
            if (c == '\'' || c == '\n' || c == '\r') return i;
        }
        return null;
    }

    // ========================================================================
    // Security Guards: Anchors, Circular Alias & Expansion Budget
    // ========================================================================

    static fn countNodesBounded(val: ConfValue, limit: usize, depth: usize): usize? {
        if (depth > YAML_MAX_DEPTH) return null;
        if (limit == 0) return null;
        let total: usize = 1;
        switch (val.tag) {
            .Array => {
                for (item of val.arr_val) {
                    if (total >= limit) return null;
                    const sub = YamlParser.countNodesBounded(item, limit - total, depth + 1) orelse return null;
                    total += sub;
                    if (total > limit) return null;
                }
            },
            .Object => {
                for (f of val.obj_val) {
                    if (total >= limit) return null;
                    const sub = YamlParser.countNodesBounded(f.value, limit - total, depth + 1) orelse return null;
                    total += sub;
                    if (total > limit) return null;
                }
            },
            else => {},
        }
        return total;
    }

    fn chargeExpansionBudget(*_, val: ConfValue): ! {
        if (_.alias_expansions >= YAML_MAX_ALIAS_EXPANSIONS) {
            return error.AliasExpansionLimitExceeded;
        }
        if (_.expanded_nodes >= YAML_MAX_EXPANDED_NODES) {
            return error.AliasExpansionLimitExceeded;
        }
        const remaining = YAML_MAX_EXPANDED_NODES - _.expanded_nodes;
        const needed = YamlParser.countNodesBounded(val, remaining, 0) orelse return error.AliasExpansionLimitExceeded;
        _.alias_expansions += 1;
        _.expanded_nodes += needed;
    }

    fn isAnchorActive(name: []const u8): bool {
        for (a of _.active_anchors.items) {
            if (std.mem.eql(u8, a, name)) return true;
        }
        return false;
    }

    fn setAnchor(*_, name: []const u8, val: ConfValue): ! {
        for (*a of _.anchors.items) {
            if (std.mem.eql(u8, a.name, name)) {
                a.value = val;
                return;
            }
        }
        try _.anchors.append(_.allocator, YamlAnchor{ name: name, value: val });
    }

    fn lookupAnchor(name: []const u8): ConfValue? {
        let i: usize = _.anchors.items.len;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.anchors.items[i].name, name)) {
                return _.anchors.items[i].value;
            }
        }
        return null;
    }

    // ========================================================================
    // Cursor, Whitespace & Line Indentation Helpers (SIMD-Accelerated)
    // ========================================================================

    fn isEof(): bool {
        return _.pos >= _.src.len;
    }

    fn peekAt(offset: usize): u8 {
        if (_.pos + offset >= _.src.len) return 0;
        return _.src[_.pos + offset];
    }

    fn skipHorizontalWs(*_) {
        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
            _.pos += 1;
        }
    }

    fn skipLineComment(*_) {
        if (_.pos < _.src.len && _.src[_.pos] == '#') {
            const rest = _.src[_.pos..];
            if (nl := Simd.indexOfByte(rest, '\n')) {
                _.pos += nl;
            } else {
                _.pos = _.src.len;
            }
        }
    }

    fn skipToNextLine(*_) {
        const rest = _.src[_.pos..];
        if (nl := Simd.indexOfByte(rest, '\n')) {
            _.pos += nl + 1;
        } else {
            _.pos = _.src.len;
        }
    }

    fn skipBlankAndCommentLines(*_) {
        while (_.pos < _.src.len) {
            const saved = _.pos;
            _.skipHorizontalWs();
            if (_.pos >= _.src.len) break;
            if (_.src[_.pos] == '\r') {
                _.pos += 1;
                if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
                continue;
            }
            if (_.src[_.pos] == '\n') {
                _.pos += 1;
                continue;
            }
            if (_.src[_.pos] == '#') {
                _.skipToNextLine();
                continue;
            }
            _.pos = saved;
            break;
        }
    }

    fn currentLineStart(): usize {
        let line_start = _.pos;
        while (line_start > 0 && _.src[line_start - 1] != '\n') {
            line_start -= 1;
        }
        return line_start;
    }

    fn currentLineIndent(): usize {
        const line_start = _.currentLineStart();
        return YamlParser.countIndentSpacesSimd(_.src[line_start..]);
    }

    fn checkBlockLineIndent(): usize! {
        const line_start = _.currentLineStart();
        const sp = YamlParser.countIndentSpacesSimd(_.src[line_start..]);
        if (line_start + sp < _.src.len && _.src[line_start + sp] == '\t') {
            return error.TabIndentationForbidden;
        }
        return sp;
    }

    fn currentColumn(): usize {
        return _.pos - _.currentLineStart();
    }

    fn expectLineEndOrComment(*_): ! {
        _.skipHorizontalWs();
        if (_.pos >= _.src.len) return;
        const c = _.src[_.pos];
        if (c == '#') {
            _.skipToNextLine();
            return;
        }
        if (c == '\r') {
            _.pos += 1;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
            return;
        }
        if (c == '\n') {
            _.pos += 1;
            return;
        }
        return error.SyntaxError;
    }

    fn isDocStartAtLine(): bool {
        if (_.pos + 3 > _.src.len) return false;
        if (_.src[_.pos] == '-' && _.src[_.pos + 1] == '-' && _.src[_.pos + 2] == '-') {
            if (_.pos + 3 == _.src.len) return true;
            const c = _.src[_.pos + 3];
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }
        return false;
    }

    fn isDocEndAtLine(): bool {
        if (_.pos + 3 > _.src.len) return false;
        if (_.src[_.pos] == '.' && _.src[_.pos + 1] == '.' && _.src[_.pos + 2] == '.') {
            if (_.pos + 3 == _.src.len) return true;
            const c = _.src[_.pos + 3];
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }
        return false;
    }

    // ========================================================================
    // Multi-Document Stream Driver
    // ========================================================================

    fn runStream(*_): YamlStream! {
        let docs = YamlValueList.empty;

        // Strip UTF-8 BOM if present
        if (_.src.len >= 3 && _.src[0] == 0xEF && _.src[1] == 0xBB && _.src[2] == 0xBF) {
            _.pos = 3;
        }

        while (true) {
            _.skipBlankAndCommentLines();
            if (_.pos >= _.src.len) break;

            // Skip directives (%YAML / %TAG)
            while (_.pos < _.src.len && _.src[_.pos] == '%') {
                _.skipToNextLine();
                _.skipBlankAndCommentLines();
            }
            if (_.pos >= _.src.len) break;

            // Consume optional `...`
            if (_.isDocEndAtLine()) {
                _.skipToNextLine();
                continue;
            }

            // Reset per-document anchors when entering a new `---` document
            let had_explicit_start = false;
            if (_.isDocStartAtLine()) {
                _.anchors.clearRetainingCapacity();
                _.pos += 3;
                _.skipHorizontalWs();
                had_explicit_start = true;
            }

            _.skipBlankAndCommentLines();
            if (_.pos >= _.src.len) {
                if (had_explicit_start) {
                    try docs.append(_.allocator, ConfValue.makeNull());
                }
                break;
            }
            if (_.isDocEndAtLine()) {
                if (had_explicit_start) {
                    try docs.append(_.allocator, ConfValue.makeNull());
                }
                _.skipToNextLine();
                continue;
            }
            if (_.isDocStartAtLine()) {
                if (had_explicit_start) {
                    try docs.append(_.allocator, ConfValue.makeNull());
                }
                continue;
            }

            if (_.currentColumn() == 0) {
                * = try _.checkBlockLineIndent();
            }

            const pos_before = _.pos;
            const doc_val = try _.parseNode(0, false, false);
            try docs.append(_.allocator, doc_val);

            // Prevent infinite loop on malformed unconsumed input
            if (_.pos == pos_before) {
                return error.SyntaxError;
            }

            // If still on the same line after a scalar/flow document, validate no trailing garbage
            if (_.pos < _.src.len && _.currentColumn() > 0) {
                try _.expectLineEndOrComment();
            }

            _.skipBlankAndCommentLines();
            if (_.pos < _.src.len && _.isDocEndAtLine()) {
                _.skipToNextLine();
            } else if (_.pos < _.src.len && !_.isDocStartAtLine()) {
                // Unexpected trailing content without `---` document separator
                return error.SyntaxError;
            }
        }

        return YamlStream{
            docs: try docs.toOwnedSlice(_.allocator),
        };
    }

    // ========================================================================
    // Properties (Anchors & Explicit Tags)
    // ========================================================================

    fn parseProperties(*_): NodeProps! {
        let props = NodeProps{};
        while (true) {
            _.skipHorizontalWs();
            if (_.pos >= _.src.len) break;
            const c = _.src[_.pos];
            if (c == '&') {
                _.pos += 1;
                const start = _.pos;
                while (_.pos < _.src.len && !isYamlSpaceOrBreak(_.src[_.pos]) &&
                    _.src[_.pos] != ',' && _.src[_.pos] != ']' && _.src[_.pos] != '}' && _.src[_.pos] != ':')
                {
                    _.pos += 1;
                }
                if (_.pos == start) return error.SyntaxError;
                props.anchor = _.src[start.._.pos];
            } else if (c == '!') {
                const start = _.pos;
                while (_.pos < _.src.len && !isYamlSpaceOrBreak(_.src[_.pos]) &&
                    _.src[_.pos] != ',' && _.src[_.pos] != ']' && _.src[_.pos] != '}')
                {
                    _.pos += 1;
                }
                const tag_str = _.src[start.._.pos];
                if (std.mem.eql(u8, tag_str, "!!str") || std.mem.eql(u8, tag_str, "!<tag:yaml.org,2002:str>")) {
                    props.tag = YamlTagOverride.Str;
                } else if (std.mem.eql(u8, tag_str, "!!int") || std.mem.eql(u8, tag_str, "!<tag:yaml.org,2002:int>")) {
                    props.tag = YamlTagOverride.Int;
                } else if (std.mem.eql(u8, tag_str, "!!float") || std.mem.eql(u8, tag_str, "!<tag:yaml.org,2002:float>")) {
                    props.tag = YamlTagOverride.Float;
                } else if (std.mem.eql(u8, tag_str, "!!bool") || std.mem.eql(u8, tag_str, "!<tag:yaml.org,2002:bool>")) {
                    props.tag = YamlTagOverride.Bool;
                } else if (std.mem.eql(u8, tag_str, "!!null") || std.mem.eql(u8, tag_str, "!<tag:yaml.org,2002:null>")) {
                    props.tag = YamlTagOverride.Null;
                } else if (std.mem.eql(u8, tag_str, "!!seq")) {
                    props.tag = YamlTagOverride.Seq;
                } else if (std.mem.eql(u8, tag_str, "!!map")) {
                    props.tag = YamlTagOverride.Map;
                }
            } else {
                break;
            }
        }
        return props;
    }

    static fn applyTag(val: ConfValue, tag: YamlTagOverride, allocator: std.mem.Allocator): ConfValue! {
        return switch (tag) {
            .None => val,
            .Null => ConfValue.makeNull(),
            .Str => switch (val.tag) {
                .String => val,
                .Null => ConfValue.makeString(""),
                .Bool => ConfValue.makeString(if (val.bool_val) "true" else "false"),
                .Int => ConfValue.makeString(try std.fmt.allocPrint(allocator, "{d}", .{val.int_val})),
                .Float => ConfValue.makeString(try std.fmt.allocPrint(allocator, "{d}", .{val.float_val})),
                else => val,
            },
            .Int => switch (val.tag) {
                .Int => val,
                .String => {
                    if (n := parseYamlInt(val.str_val)) return ConfValue.makeInt(n);
                    return error.InvalidTag;
                },
                .Float => ConfValue.makeInt(val.float_val as i64),
                else => return error.InvalidTag,
            },
            .Float => switch (val.tag) {
                .Float => val,
                .Int => ConfValue.makeFloat(val.int_val as f64),
                .String => {
                    if (f := parseYamlFloat(val.str_val)) return ConfValue.makeFloat(f);
                    return error.InvalidTag;
                },
                else => return error.InvalidTag,
            },
            .Bool => switch (val.tag) {
                .Bool => val,
                .String => {
                    if (b := parseYamlBoolExtended(val.str_val)) return ConfValue.makeBool(b);
                    return error.InvalidTag;
                },
                else => return error.InvalidTag,
            },
            else => val,
        };
    }

    // ========================================================================
    // Core Recursive Node Parser
    // ========================================================================

    fn parseNode(*_, min_indent: usize, in_flow: bool, in_block_collection: bool): ConfValue! {
        if (_.depth >= YAML_MAX_DEPTH) return error.DepthExceeded;

        if (!in_flow) {
            _.skipBlankAndCommentLines();
        } else {
            _.skipFlowWs();
        }
        if (_.pos >= _.src.len) return ConfValue.makeNull();
        if (!in_flow && (_.isDocStartAtLine() || _.isDocEndAtLine())) {
            return ConfValue.makeNull();
        }

        const props = try _.parseProperties();
        let pushed_active_anchor = false;
        if (aname := props.anchor) {
            try _.active_anchors.append(_.allocator, aname);
            pushed_active_anchor = true;
        }
        defer {
            if (pushed_active_anchor) {
                * = _.active_anchors.pop();
            }
        }

        if (!in_flow) {
            _.skipHorizontalWs();
        } else {
            _.skipFlowWs();
        }

        let val = ConfValue.makeNull();

        if (_.pos >= _.src.len) {
            val = ConfValue.makeNull();
        } else {
            const c = _.src[_.pos];
            if (!in_flow && c == '#') {
                _.skipToNextLine();
                _.skipBlankAndCommentLines();
                if (_.pos < _.src.len && !_.isDocStartAtLine() && !_.isDocEndAtLine()) {
                    const next_ind = try _.checkBlockLineIndent();
                    if (next_ind > min_indent || (next_ind == min_indent && isSeqMarker(_.src, _.pos + next_ind))) {
                        val = try _.parseBlockFromCurrent(next_ind);
                    }
                }
            } else if (!in_flow && (c == '\n' || c == '\r')) {
                _.skipToNextLine();
                _.skipBlankAndCommentLines();
                if (_.pos < _.src.len && !_.isDocStartAtLine() && !_.isDocEndAtLine()) {
                    const next_ind = try _.checkBlockLineIndent();
                    if (next_ind > min_indent || (next_ind == min_indent && isSeqMarker(_.src, _.pos + next_ind))) {
                        val = try _.parseBlockFromCurrent(next_ind);
                    }
                }
            } else if (c == '*') {
                _.pos += 1;
                const astart = _.pos;
                while (_.pos < _.src.len && !isYamlSpaceOrBreak(_.src[_.pos]) &&
                    _.src[_.pos] != ',' && _.src[_.pos] != ']' && _.src[_.pos] != '}')
                {
                    _.pos += 1;
                }
                const aname = _.src[astart.._.pos];
                if (aname.len == 0) return error.UnknownAlias;
                if (_.isAnchorActive(aname)) return error.CircularAlias;
                const found = _.lookupAnchor(aname) orelse return error.UnknownAlias;
                try _.chargeExpansionBudget(found);
                val = try found.clone(_.allocator);
            } else if (c == '[') {
                val = try _.parseFlowSeq();
            } else if (c == '{') {
                val = try _.parseFlowMap();
            } else if (!in_flow && (c == '|' || c == '>')) {
                val = try _.parseBlockScalar(min_indent, in_block_collection);
            } else if (!in_flow && isSeqMarker(_.src, _.pos)) {
                const seq_col = _.currentColumn();
                val = try _.parseBlockSequence(seq_col);
            } else if (!in_flow && _.looksLikeBlockMappingLine()) {
                const map_col = _.currentColumn();
                val = try _.parseBlockMapping(map_col);
            } else if (c == '"') {
                const s = try _.parseDoubleQuoted();
                val = ConfValue.makeString(s);
            } else if (c == '\'') {
                const s = try _.parseSingleQuoted();
                val = ConfValue.makeString(s);
            } else {
                const raw = _.scanPlainScalar(in_flow);
                val = resolvePlainScalar(raw);
            }
        }

        val = try YamlParser.applyTag(val, props.tag, _.allocator);
        if (aname := props.anchor) {
            try _.setAnchor(aname, val);
        }
        return val;
    }

    fn parseBlockFromCurrent(*_, block_indent: usize): ConfValue! {
        if (_.depth >= YAML_MAX_DEPTH) return error.DepthExceeded;
        * = try _.checkBlockLineIndent();
        _.skipHorizontalWs();
        if (_.pos >= _.src.len) return ConfValue.makeNull();
        if (isSeqMarker(_.src, _.pos)) {
            return try _.parseBlockSequence(_.currentColumn());
        }
        if (_.looksLikeBlockMappingLine()) {
            return try _.parseBlockMapping(_.currentColumn());
        }
        return try _.parseNode(block_indent, false, true);
    }

    fn looksLikeBlockMappingLine(): bool {
        let i = _.pos;
        if (i >= _.src.len) return false;
        const first = _.src[i];
        if (first == '[' || first == '{' || first == '|' || first == '>' || first == '*') return false;
        if (first == '"' || first == '\'') {
            const q = first;
            i += 1;
            while (i < _.src.len && _.src[i] != '\n') {
                if (q == '"' && _.src[i] == '\\') {
                    i += 2;
                    continue;
                }
                if (q == '\'' && _.src[i] == '\'' && i + 1 < _.src.len && _.src[i + 1] == '\'') {
                    i += 2;
                    continue;
                }
                if (_.src[i] == q) {
                    i += 1;
                    break;
                }
                i += 1;
            }
            while (i < _.src.len && (_.src[i] == ' ' || _.src[i] == '\t')) i += 1;
            return i < _.src.len && _.src[i] == ':' && (i + 1 >= _.src.len || isYamlSpaceOrBreak(_.src[i + 1]));
        }

        while (i < _.src.len && _.src[i] != '\n' && _.src[i] != '\r') {
            if (_.src[i] == '#') break;
            if (_.src[i] == ':' && (i + 1 >= _.src.len || isYamlSpaceOrBreak(_.src[i + 1]))) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    // ========================================================================
    // Block Sequence & Block Mapping
    // ========================================================================

    fn parseBlockSequence(*_, seq_indent: usize): ConfValue! {
        if (_.depth >= YAML_MAX_DEPTH) return error.DepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        let items = YamlValueList.empty;

        while (true) {
            _.skipBlankAndCommentLines();
            if (_.pos >= _.src.len || _.isDocStartAtLine() || _.isDocEndAtLine()) break;

            const ind = try _.checkBlockLineIndent();
            const col = _.currentColumn();
            if (col == 0) {
                if (ind != seq_indent) break;
                _.pos += ind;
            } else if (col != seq_indent) {
                break;
            }
            if (!isSeqMarker(_.src, _.pos)) break;
            _.pos += 1; // consume '-'

            _.skipHorizontalWs();

            if (_.pos >= _.src.len || _.src[_.pos] == '\n' || _.src[_.pos] == '\r' || _.src[_.pos] == '#') {
                _.skipLineComment();
                if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                    _.skipToNextLine();
                }
                _.skipBlankAndCommentLines();
                if (_.pos < _.src.len && !_.isDocStartAtLine() && !_.isDocEndAtLine()) {
                    const next_ind = try _.checkBlockLineIndent();
                    if (next_ind > seq_indent) {
                        const child = try _.parseBlockFromCurrent(next_ind);
                        try items.append(_.allocator, child);
                    } else {
                        try items.append(_.allocator, ConfValue.makeNull());
                    }
                } else {
                    try items.append(_.allocator, ConfValue.makeNull());
                }
            } else if (isSeqMarker(_.src, _.pos)) {
                // Compact nested sequence: `- - item`
                const nested_col = _.currentColumn();
                const child = try _.parseBlockSequence(nested_col);
                try items.append(_.allocator, child);
            } else if (_.looksLikeBlockMappingLine()) {
                // Compact block mapping inside sequence: `- key: val`
                const map_col = _.currentColumn();
                const child = try _.parseBlockMapping(map_col);
                try items.append(_.allocator, child);
            } else {
                const elem = try _.parseNode(seq_indent, false, true);
                try items.append(_.allocator, elem);
                if (_.pos < _.src.len && _.currentColumn() > 0) {
                    try _.expectLineEndOrComment();
                }
            }
        }

        return ConfValue.makeArray(try items.toOwnedSlice(_.allocator));
    }

    fn mergeObjectInto(*_, fields: *YamlFieldList, src_obj: ConfValue): ! {
        if (src_obj.tag != ConfTag.Object) return;
        for (sf of src_obj.obj_val) {
            let exists = false;
            for (ef of fields.items) {
                if (std.mem.eql(u8, ef.key, sf.key)) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                try _.chargeExpansionBudget(sf.value);
                try fields.append(_.allocator, ConfField{
                    key: try _.allocator.dupe(u8, sf.key),
                    value: try sf.value.clone(_.allocator),
                });
            }
        }
    }

    fn parseMappingKey(*_): ([]const u8)! {
        _.skipHorizontalWs();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        if (_.src[_.pos] == '"') {
            return try _.parseDoubleQuoted();
        }
        if (_.src[_.pos] == '\'') {
            return try _.parseSingleQuoted();
        }
        const start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
            if (_.src[_.pos] == ':' && (_.pos + 1 >= _.src.len || isYamlSpaceOrBreak(_.src[_.pos + 1]))) {
                break;
            }
            _.pos += 1;
        }
        return std.mem.trimEnd(u8, _.src[start.._.pos], " \t");
    }

    static fn upsertField(fields: *YamlFieldList, key: []const u8, val: ConfValue, allocator: std.mem.Allocator): ! {
        for (*ef of fields.items) {
            if (std.mem.eql(u8, ef.key, key)) {
                ef.value = val;
                return;
            }
        }
        try fields.append(allocator, ConfField{ key: key, value: val });
    }

    fn parseBlockMapping(*_, map_indent: usize): ConfValue! {
        if (_.depth >= YAML_MAX_DEPTH) return error.DepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        let fields = YamlFieldList.empty;
        let merge_sources = YamlValueList.empty;

        while (true) {
            _.skipBlankAndCommentLines();
            if (_.pos >= _.src.len || _.isDocStartAtLine() || _.isDocEndAtLine()) break;

            const line_ind = try _.checkBlockLineIndent();
            // Handle compact first line where cursor is already at `map_indent` past line start
            const col = _.currentColumn();
            if (col == 0) {
                if (line_ind != map_indent) break;
                _.pos += line_ind;
            } else if (col != map_indent) {
                break;
            }

            if (!_.looksLikeBlockMappingLine()) break;

            const key = try _.parseMappingKey();
            _.skipHorizontalWs();
            if (_.pos >= _.src.len || _.src[_.pos] != ':') return error.SyntaxError;
            _.pos += 1; // consume ':'
            _.skipHorizontalWs();

            let val = ConfValue.makeNull();
            if (_.pos >= _.src.len || _.src[_.pos] == '\n' || _.src[_.pos] == '\r' || _.src[_.pos] == '#') {
                _.skipLineComment();
                if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                    _.skipToNextLine();
                }
                _.skipBlankAndCommentLines();
                if (_.pos < _.src.len && !_.isDocStartAtLine() && !_.isDocEndAtLine()) {
                    const next_ind = try _.checkBlockLineIndent();
                    if (next_ind > map_indent) {
                        val = try _.parseBlockFromCurrent(next_ind);
                    } else if (next_ind == map_indent && isSeqMarkerAt(_.src, _.pos + next_ind)) {
                        _.pos += next_ind;
                        val = try _.parseBlockSequence(next_ind);
                    }
                }
            } else {
                val = try _.parseNode(map_indent, false, true);
                if (_.pos < _.src.len && _.currentColumn() > 0) {
                    try _.expectLineEndOrComment();
                }
            }

            if (std.mem.eql(u8, key, "<<")) {
                if (val.tag == ConfTag.Object) {
                    try merge_sources.append(_.allocator, val);
                } else if (val.tag == ConfTag.Array) {
                    for (item of val.arr_val) {
                        if (item.tag == ConfTag.Object) {
                            try merge_sources.append(_.allocator, item);
                        }
                    }
                }
            } else {
                try YamlParser.upsertField(&fields, key, val, _.allocator);
            }
        }

        // Apply YAML merge keys (explicit keys take precedence over merged keys)
        for (ms of merge_sources.items) {
            try _.mergeObjectInto(&fields, ms);
        }
        merge_sources.deinit(_.allocator);

        return ConfValue.makeObject(try fields.toOwnedSlice(_.allocator));
    }

    // ========================================================================
    // Block Scalars (`|`, `|-`, `|+`, `>`, `>-`, `>+`, explicit indent `1..9`)
    // ========================================================================

    fn parseBlockScalar(*_, parent_indent: usize, in_block_collection: bool): ConfValue! {
        const style = _.src[_.pos]; // '|' or '>'
        _.pos += 1;

        let chomp: u8 = 0; // 0=clip, 1=strip(-), 2=keep(+)
        let has_chomp = false;
        let explicit_indent: usize = 0;
        let seen_ws = false;

        while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r' && _.src[_.pos] != '#') {
            const c = _.src[_.pos];
            if (!seen_ws && (c == '-' || c == '+')) {
                if (has_chomp) return error.SyntaxError;
                chomp = if (c == '-') 1 else 2;
                has_chomp = true;
            } else if (!seen_ws && c >= '1' && c <= '9') {
                if (explicit_indent != 0) return error.SyntaxError;
                explicit_indent = (c - '0') as usize;
            } else if (c == ' ' || c == '\t') {
                seen_ws = true;
            } else {
                return error.SyntaxError;
            }
            _.pos += 1;
        }
        _.skipLineComment();
        if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
            _.skipToNextLine();
        }

        const min_required_indent: usize = if (in_block_collection) (parent_indent + 1) else 0;
        let content_indent: usize = if (explicit_indent > 0) (parent_indent + explicit_indent) else 0;

        if (content_indent == 0) {
            let scan_pos = _.pos;
            let found_content = false;
            let leading_blank_count: usize = 0;
            while (scan_pos < _.src.len) {
                const sp = YamlParser.countIndentSpacesSimd(_.src[scan_pos..]);
                const after_sp = scan_pos + sp;
                if (after_sp >= _.src.len || _.src[after_sp] == '\n' || _.src[after_sp] == '\r') {
                    leading_blank_count += 1;
                    scan_pos = after_sp;
                    if (scan_pos < _.src.len && _.src[scan_pos] == '\r') scan_pos += 1;
                    if (scan_pos < _.src.len && _.src[scan_pos] == '\n') scan_pos += 1;
                    continue;
                }
                if (sp == 0 && (isDocStartAtSlice(_.src, scan_pos) || isDocEndAtSlice(_.src, scan_pos))) {
                    break;
                }
                content_indent = sp;
                found_content = true;
                break;
            }
            if (!found_content || content_indent < min_required_indent) {
                // Consume only blank lines belonging to empty block scalar
                if (chomp == 2 && leading_blank_count > 0) {
                    let consumed: usize = 0;
                    while (consumed < leading_blank_count && _.pos < _.src.len) : (consumed += 1) {
                        _.skipToNextLine();
                    }
                    let buf = try _.allocator.alloc(u8, leading_blank_count);
                    let k: usize = 0;
                    while (k < leading_blank_count) : (k += 1) buf[k] = '\n';
                    return ConfValue.makeString(buf);
                }
                return ConfValue.makeString("");
            }
        }

        let lines = YamlStrList.empty;
        defer lines.deinit(_.allocator);
        let trailing_blanks: usize = 0;

        while (_.pos < _.src.len) {
            if (_.isDocStartAtLine() || _.isDocEndAtLine()) break;
            const sp = YamlParser.countIndentSpacesSimd(_.src[_.pos..]);
            const after_sp = _.pos + sp;
            const is_blank = (after_sp >= _.src.len || _.src[after_sp] == '\n' || _.src[after_sp] == '\r');
            if (!is_blank && sp < content_indent) {
                break;
            }

            const line_end = after_sp + YamlParser.scanLineEndSimd(_.src[after_sp..]);

            if (is_blank) {
                const extra = if (sp > content_indent) _.src[_.pos + content_indent .. line_end] else "";
                try lines.append(_.allocator, extra);
                trailing_blanks += 1;
            } else {
                trailing_blanks = 0;
                try lines.append(_.allocator, _.src[_.pos + content_indent .. line_end]);
            }

            _.pos = line_end;
            if (_.pos < _.src.len && _.src[_.pos] == '\r') _.pos += 1;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
        }

        const content_line_count = lines.items.len - trailing_blanks;
        if (content_line_count == 0) {
            if (chomp == 2 && trailing_blanks > 0) {
                let buf = try _.allocator.alloc(u8, trailing_blanks);
                let k: usize = 0;
                while (k < trailing_blanks) : (k += 1) buf[k] = '\n';
                return ConfValue.makeString(buf);
            }
            return ConfValue.makeString("");
        }

        let out = YamlByteList.empty;
        if (style == '|') {
            let i: usize = 0;
            while (i < content_line_count) : (i += 1) {
                if (i > 0) try out.append(_.allocator, '\n');
                try out.appendSlice(_.allocator, lines.items[i]);
            }
        } else {
            // YAML 1.2 Folded '>' (Spec 8.1.3):
            // - Adjacent normal lines fold with ' '
            // - More-indented lines preserve newlines before & after (including around blank lines)
            // - Blank lines between normal lines emit one '\n' per blank line
            let i: usize = 0;
            let prev_was_indented = false;
            let had_non_empty = false;
            while (i < content_line_count) : (i += 1) {
                const ln = lines.items[i];
                const is_empty = (ln.len == 0);
                const is_indented = (!is_empty && (ln[0] == ' ' || ln[0] == '\t'));
                if (is_empty) {
                    if (had_non_empty) {
                        if (prev_was_indented) {
                            try out.append(_.allocator, '\n');
                            prev_was_indented = false;
                        }
                        try out.append(_.allocator, '\n');
                    } else {
                        try out.append(_.allocator, '\n');
                    }
                    continue;
                }
                if (i > 0) {
                    const prev_empty = (lines.items[i - 1].len == 0);
                    if (!prev_empty) {
                        if (is_indented || prev_was_indented) {
                            try out.append(_.allocator, '\n');
                        } else {
                            try out.append(_.allocator, ' ');
                        }
                    }
                }
                try out.appendSlice(_.allocator, ln);
                had_non_empty = true;
                prev_was_indented = is_indented;
            }
        }

        if (chomp == 0) {
            try out.append(_.allocator, '\n');
        } else if (chomp == 2) {
            try out.append(_.allocator, '\n');
            let k: usize = 0;
            while (k < trailing_blanks) : (k += 1) {
                try out.append(_.allocator, '\n');
            }
        }

        return ConfValue.makeString(try out.toOwnedSlice(_.allocator));
    }

    // ========================================================================
    // Flow Sequences & Flow Mappings (SIMD Whitespace + Depth Guard)
    // ========================================================================

    fn skipFlowWs(*_) {
        while (_.pos < _.src.len) {
            const adv = Simd.skipWhitespace(_.src[_.pos..]);
            _.pos += adv;
            if (_.pos < _.src.len && _.src[_.pos] == '#') {
                _.skipToNextLine();
            } else {
                break;
            }
        }
    }

    fn parseFlowSeq(*_): ConfValue! {
        if (_.depth >= YAML_MAX_DEPTH) return error.DepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // '['
        let items = YamlValueList.empty;
        while (true) {
            _.skipFlowWs();
            if (_.pos >= _.src.len) return error.UnexpectedEof;
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            if (_.src[_.pos] == ',') {
                return error.SyntaxError;
            }
            const elem = try _.parseNode(0, true, false);
            try items.append(_.allocator, elem);
            _.skipFlowWs();
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
        return ConfValue.makeArray(try items.toOwnedSlice(_.allocator));
    }

    fn parseFlowMap(*_): ConfValue! {
        if (_.depth >= YAML_MAX_DEPTH) return error.DepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // '{'
        let fields = YamlFieldList.empty;
        let merge_sources = YamlValueList.empty;

        while (true) {
            _.skipFlowWs();
            if (_.pos >= _.src.len) return error.UnexpectedEof;
            if (_.src[_.pos] == '}') {
                _.pos += 1;
                break;
            }
            if (_.src[_.pos] == ',') {
                return error.SyntaxError;
            }

            let key: []const u8 = "";
            if (_.src[_.pos] == '"') {
                key = try _.parseDoubleQuoted();
            } else if (_.src[_.pos] == '\'') {
                key = try _.parseSingleQuoted();
            } else {
                const start = _.pos;
                while (_.pos < _.src.len && _.src[_.pos] != ':' && _.src[_.pos] != ',' && _.src[_.pos] != '}' && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                    _.pos += 1;
                }
                if (_.pos == start) return error.SyntaxError;
                key = std.mem.trim(u8, _.src[start.._.pos], " \t");
            }

            _.skipFlowWs();
            let val = ConfValue.makeNull();
            if (_.pos < _.src.len && _.src[_.pos] == ':') {
                _.pos += 1;
                _.skipFlowWs();
                if (_.pos < _.src.len && _.src[_.pos] != ',' && _.src[_.pos] != '}') {
                    val = try _.parseNode(0, true, false);
                }
            }

            if (std.mem.eql(u8, key, "<<")) {
                if (val.tag == ConfTag.Object) {
                    try merge_sources.append(_.allocator, val);
                } else if (val.tag == ConfTag.Array) {
                    for (item of val.arr_val) {
                        if (item.tag == ConfTag.Object) try merge_sources.append(_.allocator, item);
                    }
                }
            } else {
                try YamlParser.upsertField(&fields, key, val, _.allocator);
            }

            _.skipFlowWs();
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

        for (ms of merge_sources.items) {
            try _.mergeObjectInto(&fields, ms);
        }
        merge_sources.deinit(_.allocator);
        return ConfValue.makeObject(try fields.toOwnedSlice(_.allocator));
    }

    // ========================================================================
    // Quoted & Plain Scalars (SIMD-Accelerated Fast Paths)
    // ========================================================================

    fn parseSingleQuoted(*_): ([]const u8)! {
        _.pos += 1; // opening '\''
        let out = YamlByteList.empty;
        while (_.pos < _.src.len) {
            const rest = _.src[_.pos..];
            const stop = YamlParser.scanSingleQuotedStopSimd(rest) orelse return error.UnexpectedEof;
            if (stop > 0) {
                try out.appendSlice(_.allocator, rest[0..stop]);
                _.pos += stop;
            }
            const c = _.src[_.pos];
            if (c == '\'') {
                if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\'') {
                    try out.append(_.allocator, '\'');
                    _.pos += 2;
                    continue;
                }
                _.pos += 1;
                return try out.toOwnedSlice(_.allocator);
            }
            if (c == '\r' || c == '\n') {
                if (c == '\r' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') _.pos += 1;
                _.pos += 1;
                _.skipHorizontalWs();
                try out.append(_.allocator, ' ');
                continue;
            }
        }
        return error.UnexpectedEof;
    }

    static fn parseHexDigits(src: []const u8): u32? {
        let acc: u32 = 0;
        for (c of src) {
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

    static fn appendUtf8(out: *YamlByteList, allocator: std.mem.Allocator, cp: u32): ! {
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return error.InvalidEscape;
        let buf: [4]u8 = undefined;
        const n = std.unicode.utf8Encode(@intCast(cp), &buf) catch return error.InvalidEscape;
        try out.appendSlice(allocator, buf[0..n]);
    }

    fn parseDoubleQuoted(*_): ([]const u8)! {
        _.pos += 1; // opening '"'
        let out = YamlByteList.empty;
        while (_.pos < _.src.len) {
            const rest = _.src[_.pos..];
            const stop = YamlParser.scanDoubleQuotedStopSimd(rest) orelse return error.UnexpectedEof;
            if (stop > 0) {
                try out.appendSlice(_.allocator, rest[0..stop]);
                _.pos += stop;
            }
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                return try out.toOwnedSlice(_.allocator);
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.src.len) return error.UnexpectedEof;
                const esc = _.src[_.pos];
                _.pos += 1;
                switch (esc) {
                    '0' => try out.append(_.allocator, 0),
                    'a' => try out.append(_.allocator, 0x07),
                    'b' => try out.append(_.allocator, 0x08),
                    't', '\t' => try out.append(_.allocator, '\t'),
                    'n' => try out.append(_.allocator, '\n'),
                    'v' => try out.append(_.allocator, 0x0B),
                    'f' => try out.append(_.allocator, 0x0C),
                    'r' => try out.append(_.allocator, '\r'),
                    'e' => try out.append(_.allocator, 0x1B),
                    ' ' => try out.append(_.allocator, ' '),
                    '"' => try out.append(_.allocator, '"'),
                    '/' => try out.append(_.allocator, '/'),
                    '\\' => try out.append(_.allocator, '\\'),
                    'N' => try YamlParser.appendUtf8(&out, _.allocator, 0x0085),
                    '_' => try YamlParser.appendUtf8(&out, _.allocator, 0x00A0),
                    'L' => try YamlParser.appendUtf8(&out, _.allocator, 0x2028),
                    'P' => try YamlParser.appendUtf8(&out, _.allocator, 0x2029),
                    '\n' => {
                        _.skipHorizontalWs();
                    },
                    '\r' => {
                        if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
                        _.skipHorizontalWs();
                    },
                    'x' => {
                        if (_.pos + 2 > _.src.len) return error.InvalidEscape;
                        const cp = YamlParser.parseHexDigits(_.src[_.pos .. _.pos + 2]) orelse return error.InvalidEscape;
                        _.pos += 2;
                        try YamlParser.appendUtf8(&out, _.allocator, cp);
                    },
                    'u' => {
                        if (_.pos + 4 > _.src.len) return error.InvalidEscape;
                        const cp = YamlParser.parseHexDigits(_.src[_.pos .. _.pos + 4]) orelse return error.InvalidEscape;
                        _.pos += 4;
                        try YamlParser.appendUtf8(&out, _.allocator, cp);
                    },
                    'U' => {
                        if (_.pos + 8 > _.src.len) return error.InvalidEscape;
                        const cp = YamlParser.parseHexDigits(_.src[_.pos .. _.pos + 8]) orelse return error.InvalidEscape;
                        _.pos += 8;
                        try YamlParser.appendUtf8(&out, _.allocator, cp);
                    },
                    else => return error.InvalidEscape,
                }
                continue;
            }
            if (c == '\r' || c == '\n') {
                if (c == '\r' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') _.pos += 1;
                _.pos += 1;
                _.skipHorizontalWs();
                try out.append(_.allocator, ' ');
                continue;
            }
        }
        return error.UnexpectedEof;
    }

    fn scanPlainScalar(*_, in_flow: bool): []const u8 {
        const start = _.pos;
        if (!in_flow) {
            while (_.pos < _.src.len) {
                const rest = _.src[_.pos..];
                const stop = Simd.indexOfAny4(rest, '\n', '\r', '#', '#') orelse {
                    _.pos = _.src.len;
                    break;
                };
                _.pos += stop;
                const c = _.src[_.pos];
                if (c == '\n' || c == '\r') break;
                if (c == '#' && _.pos > start && (_.src[_.pos - 1] == ' ' || _.src[_.pos - 1] == '\t')) break;
                _.pos += 1;
            }
        } else {
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (c == '\n' || c == '\r') break;
                if (c == '#' && _.pos > start && (_.src[_.pos - 1] == ' ' || _.src[_.pos - 1] == '\t')) break;
                if (c == ',' || c == ']' || c == '}') break;
                _.pos += 1;
            }
        }
        return std.mem.trim(u8, _.src[start.._.pos], " \t");
    }

    // ========================================================================
    // Bidirectional YAML 1.2 Serialization
    // ========================================================================

    pub static fn stringify(allocator: std.mem.Allocator, val: ConfValue): ([]const u8)! {
        let out = YamlByteList.empty;
        try YamlParser.writeValue(&out, allocator, val, 0);
        try out.append(allocator, '\n');
        return try out.toOwnedSlice(allocator);
    }

    pub static fn stringifyStream(allocator: std.mem.Allocator, docs: []const ConfValue): ([]const u8)! {
        let out = YamlByteList.empty;
        let i: usize = 0;
        while (i < docs.len) : (i += 1) {
            try out.appendSlice(allocator, "---\n");
            try YamlParser.writeValue(&out, allocator, docs[i], 0);
            try out.append(allocator, '\n');
        }
        return try out.toOwnedSlice(allocator);
    }

    static fn writeIndent(out: *YamlByteList, allocator: std.mem.Allocator, indent: usize): ! {
        let k: usize = 0;
        while (k < indent) : (k += 1) {
            try out.append(allocator, ' ');
        }
    }

    static fn needsQuotes(s: []const u8): bool {
        if (s.len == 0) return true;
        if (std.mem.eql(u8, s, "true") || std.mem.eql(u8, s, "True") || std.mem.eql(u8, s, "TRUE") ||
            std.mem.eql(u8, s, "false") || std.mem.eql(u8, s, "False") || std.mem.eql(u8, s, "FALSE") ||
            std.mem.eql(u8, s, "null") || std.mem.eql(u8, s, "Null") || std.mem.eql(u8, s, "NULL") ||
            std.mem.eql(u8, s, "~"))
        {
            return true;
        }
        if (s[0] == ' ' || s[s.len - 1] == ' ' || s[0] == '-' || s[0] == '#' || s[0] == '&' || s[0] == '*' || s[0] == '!' || s[0] == '[' || s[0] == '{' || s[0] == '|' || s[0] == '>' || s[0] == '\'' || s[0] == '"') {
            return true;
        }
        for (c of s) {
            if (c == ':' || c == '#' || c == '\n' || c == '\r' || c == '"' || c == '\t' || c < 0x20) return true;
        }
        if (parseYamlInt(s) != null || parseYamlFloat(s) != null) return true;
        return false;
    }

    static fn writeYamlString(out: *YamlByteList, allocator: std.mem.Allocator, s: []const u8): ! {
        if (!YamlParser.needsQuotes(s)) {
            try out.appendSlice(allocator, s);
            return;
        }
        try out.append(allocator, '"');
        for (c of s) {
            switch (c) {
                0x00 => try out.appendSlice(allocator, "\\0"),
                '"' => try out.appendSlice(allocator, "\\\""),
                '\\' => try out.appendSlice(allocator, "\\\\"),
                '\n' => try out.appendSlice(allocator, "\\n"),
                '\r' => try out.appendSlice(allocator, "\\r"),
                '\t' => try out.appendSlice(allocator, "\\t"),
                else => try out.append(allocator, c),
            }
        }
        try out.append(allocator, '"');
    }

    static fn writeValue(out: *YamlByteList, allocator: std.mem.Allocator, val: ConfValue, indent: usize): ! {
        switch (val.tag) {
            .Null => try out.appendSlice(allocator, "null"),
            .Bool => try out.appendSlice(allocator, if (val.bool_val) "true" else "false"),
            .Int => {
                const s = try std.fmt.allocPrint(allocator, "{d}", .{val.int_val});
                defer allocator.free(s);
                try out.appendSlice(allocator, s);
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    try out.appendSlice(allocator, ".nan");
                } else if (std.math.isInf(val.float_val)) {
                    try out.appendSlice(allocator, if (val.float_val < 0) "-.inf" else ".inf");
                } else {
                    const s = try std.fmt.allocPrint(allocator, "{d}", .{val.float_val});
                    defer allocator.free(s);
                    try out.appendSlice(allocator, s);
                }
            },
            .String => try YamlParser.writeYamlString(out, allocator, val.str_val),
            .DateTime => try out.appendSlice(allocator, val.dt_val.raw),
            .Array => {
                if (val.arr_val.len == 0) {
                    try out.appendSlice(allocator, "[]");
                    return;
                }
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) {
                        try out.append(allocator, '\n');
                        try YamlParser.writeIndent(out, allocator, indent);
                    }
                    const item = val.arr_val[i];
                    if ((item.tag == ConfTag.Array && item.arr_val.len > 0) || (item.tag == ConfTag.Object && item.obj_val.len > 0)) {
                        try out.appendSlice(allocator, "-\n");
                        try YamlParser.writeIndent(out, allocator, indent + 2);
                        try YamlParser.writeValue(out, allocator, item, indent + 2);
                    } else {
                        try out.appendSlice(allocator, "- ");
                        try YamlParser.writeValue(out, allocator, item, indent + 2);
                    }
                }
            },
            .Object => {
                if (val.obj_val.len == 0) {
                    try out.appendSlice(allocator, "{}");
                    return;
                }
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    if (i > 0) {
                        try out.append(allocator, '\n');
                        try YamlParser.writeIndent(out, allocator, indent);
                    }
                    const f = val.obj_val[i];
                    try YamlParser.writeYamlString(out, allocator, f.key);
                    if ((f.value.tag == ConfTag.Array && f.value.arr_val.len > 0) ||
                        (f.value.tag == ConfTag.Object && f.value.obj_val.len > 0))
                    {
                        try out.appendSlice(allocator, ":\n");
                        try YamlParser.writeIndent(out, allocator, indent + 2);
                        try YamlParser.writeValue(out, allocator, f.value, indent + 2);
                    } else {
                        try out.appendSlice(allocator, ": ");
                        try YamlParser.writeValue(out, allocator, f.value, indent + 2);
                    }
                }
            },
        }
    }
}

fn isYamlSpaceOrBreak(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDocStartAtSlice(src: []const u8, pos: usize): bool {
    if (pos + 3 > src.len) return false;
    if (src[pos] == '-' && src[pos + 1] == '-' && src[pos + 2] == '-') {
        if (pos + 3 == src.len) return true;
        const c = src[pos + 3];
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }
    return false;
}

fn isDocEndAtSlice(src: []const u8, pos: usize): bool {
    if (pos + 3 > src.len) return false;
    if (src[pos] == '.' && src[pos + 1] == '.' && src[pos + 2] == '.') {
        if (pos + 3 == src.len) return true;
        const c = src[pos + 3];
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }
    return false;
}

fn isSeqMarker(src: []const u8, pos: usize): bool {
    if (pos >= src.len || src[pos] != '-') return false;
    if (pos + 1 >= src.len) return true;
    const n = src[pos + 1];
    return n == ' ' || n == '\t' || n == '\r' || n == '\n';
}

fn isSeqMarkerAt(src: []const u8, pos: usize): bool {
    return isSeqMarker(src, pos);
}

fn parseYamlBoolExtended(s: []const u8): bool? {
    if (std.ascii.eqlIgnoreCase(s, "true") || std.ascii.eqlIgnoreCase(s, "yes") || std.ascii.eqlIgnoreCase(s, "on")) return true;
    if (std.ascii.eqlIgnoreCase(s, "false") || std.ascii.eqlIgnoreCase(s, "no") || std.ascii.eqlIgnoreCase(s, "off")) return false;
    return null;
}

fn parseYamlInt(s: []const u8): i64? {
    if (s.len == 0) return null;
    if (s.len > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return std.fmt.parseInt(i64, s[2..], 16) catch null;
    }
    if (s.len > 2 && s[0] == '0' && (s[1] == 'o' || s[1] == 'O')) {
        return std.fmt.parseInt(i64, s[2..], 8) catch null;
    }
    let clean = s;
    if (s[0] == '+') clean = s[1..];
    return std.fmt.parseInt(i64, clean, 10) catch null;
}

fn parseYamlFloat(s: []const u8): f64? {
    if (s.len == 0) return null;
    if (std.ascii.eqlIgnoreCase(s, ".inf") || std.ascii.eqlIgnoreCase(s, "+.inf")) return std.math.inf(f64);
    if (std.ascii.eqlIgnoreCase(s, "-.inf")) return -1.0 * std.math.inf(f64);
    if (std.ascii.eqlIgnoreCase(s, ".nan")) return std.math.nan(f64);
    let has_dot_or_exp = false;
    for (c of s) {
        if (c == 'x' || c == 'X' || c == 'p' || c == 'P') return null;
        if (c == '.' || c == 'e' || c == 'E') {
            has_dot_or_exp = true;
        }
    }
    if (!has_dot_or_exp) return null;
    let clean = s;
    if (s[0] == '+') clean = s[1..];
    return std.fmt.parseFloat(f64, clean) catch null;
}

fn resolvePlainScalar(raw: []const u8): ConfValue {
    if (raw.len == 0 || std.mem.eql(u8, raw, "~") ||
        std.mem.eql(u8, raw, "null") || std.mem.eql(u8, raw, "Null") || std.mem.eql(u8, raw, "NULL"))
    {
        return ConfValue.makeNull();
    }
    if (std.mem.eql(u8, raw, "true") || std.mem.eql(u8, raw, "True") || std.mem.eql(u8, raw, "TRUE")) {
        return ConfValue.makeBool(true);
    }
    if (std.mem.eql(u8, raw, "false") || std.mem.eql(u8, raw, "False") || std.mem.eql(u8, raw, "FALSE")) {
        return ConfValue.makeBool(false);
    }
    if (iv := parseYamlInt(raw)) {
        return ConfValue.makeInt(iv);
    }
    if (fv := parseYamlFloat(raw)) {
        return ConfValue.makeFloat(fv);
    }
    return ConfValue.makeString(raw);
}
