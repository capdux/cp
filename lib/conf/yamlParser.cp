// Source: Absorbed from /data/dev/bun/src/interchange/yaml.cp | Synced: 2026-11-22T12:49:17Z
// lib/conf/yaml_parser.cp
// Industrial-grade YAML 1.2 Parser & Bidirectional Serializer:
// - Multi-document streams (`---` / `...`, `%YAML`/`%TAG`)
// - Anchors & Aliases (`&anchor` / `*alias`)
// - Merge Keys (`<<: *base` and `<<: [*a, *b]`)
// - Block Scalars (`|`, `|-`, `|+`, `>`, `>-`, `>+`, explicit indent indicator)
// - Flow Sequences & Mappings (`[a, {b: 1}]`)
// - Explicit Type Tags (`!!str`, `!!int`, `!!float`, `!!bool`, `!!null`, `!!seq`, `!!map`)
// - Single & Double Quoted Escapes + YAML 1.2 Core Schema scalar resolution
// - Recursive ConfValue AST & Bidirectional YAML Serialization

const std = @import("std");
import { ConfTag, ConfField, ConfValue } from "./value";

pub error YamlError {
    OutOfMemory,
    UnexpectedEof,
    SyntaxError,
    UnknownAlias,
    InvalidEscape,
    InvalidTag,
    InvalidIndentation,
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

    pub static fn init(allocator: std.mem.Allocator, src: []const u8): YamlParser {
        return YamlParser{
            src: src,
            pos: 0,
            allocator: allocator,
            anchors: YamlAnchorList.empty,
        };
    }

    pub fn deinit(*_) {
        _.anchors.deinit(_.allocator);
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
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
        }
    }

    fn skipToNextLine(*_) {
        while (_.pos < _.src.len && _.src[_.pos] != '\n') {
            _.pos += 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
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

    fn currentLineIndent(): usize {
        let line_start = _.pos;
        while (line_start > 0 && _.src[line_start - 1] != '\n') {
            line_start -= 1;
        }
        let col: usize = 0;
        while (line_start + col < _.src.len && _.src[line_start + col] == ' ') {
            col += 1;
        }
        return col;
    }

    fn currentColumn(): usize {
        let line_start = _.pos;
        while (line_start > 0 && _.src[line_start - 1] != '\n') {
            line_start -= 1;
        }
        return _.pos - line_start;
    }

    fn isDocBoundaryAt(idx: usize): bool {
        if (idx + 3 > _.src.len) return false;
        const is_start = (_.src[idx] == '-' && _.src[idx + 1] == '-' && _.src[idx + 2] == '-');
        const is_end = (_.src[idx] == '.' && _.src[idx + 1] == '.' && _.src[idx + 2] == '.');
        if (!is_start && !is_end) return false;
        if (idx + 3 == _.src.len) return true;
        const next_ch = _.src[idx + 3];
        return next_ch == ' ' || next_ch == '\t' || next_ch == '\r' || next_ch == '\n';
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

            const doc_val = try _.parseNode(0, false);
            try docs.append(_.allocator, doc_val);

            _.skipBlankAndCommentLines();
            if (_.pos < _.src.len && _.isDocEndAtLine()) {
                _.skipToNextLine();
            }
        }

        return YamlStream{
            docs: try docs.toOwnedSlice(_.allocator),
        };
    }

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
                if (_.pos > start) {
                    props.anchor = _.src[start.._.pos];
                }
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

    fn applyTag(val: ConfValue, tag: YamlTagOverride, allocator: std.mem.Allocator): ConfValue! {
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

    fn parseNode(*_, min_indent: usize, in_flow: bool): ConfValue! {
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
                    const next_ind = _.currentLineIndent();
                    if (next_ind > min_indent || (next_ind == min_indent && _.src[_.pos] == '-' && isSeqMarker(_.src, _.pos))) {
                        val = try _.parseBlockFromCurrent(next_ind);
                    }
                }
            } else if (!in_flow && (c == '\n' || c == '\r')) {
                _.skipToNextLine();
                _.skipBlankAndCommentLines();
                if (_.pos < _.src.len && !_.isDocStartAtLine() && !_.isDocEndAtLine()) {
                    const next_ind = _.currentLineIndent();
                    if (next_ind > min_indent || (next_ind == min_indent && isSeqMarker(_.src, _.pos))) {
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
                const found = _.lookupAnchor(aname) orelse return error.UnknownAlias;
                val = try found.clone(_.allocator);
            } else if (c == '[') {
                val = try _.parseFlowSeq();
            } else if (c == '{') {
                val = try _.parseFlowMap();
            } else if (!in_flow && (c == '|' || c == '>')) {
                val = try _.parseBlockScalar(min_indent);
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
        _.skipHorizontalWs();
        if (_.pos >= _.src.len) return ConfValue.makeNull();
        if (isSeqMarker(_.src, _.pos)) {
            return try _.parseBlockSequence(_.currentColumn());
        }
        if (_.looksLikeBlockMappingLine()) {
            return try _.parseBlockMapping(_.currentColumn());
        }
        return try _.parseNode(block_indent, false);
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

    fn parseBlockSequence(*_, seq_indent: usize): ConfValue! {
        let items = YamlValueList.empty;

        while (true) {
            _.skipBlankAndCommentLines();
            if (_.pos >= _.src.len || _.isDocStartAtLine() || _.isDocEndAtLine()) break;

            const ind = _.currentLineIndent();
            const col = _.currentColumn();
            if (col == 0) {
                if (ind != seq_indent) break;
                _.pos += ind;
            } else if (col != seq_indent) {
                break;
            }
            if (!isSeqMarker(_.src, _.pos)) break;
            _.pos += 1; // consume '-'

            const item_col = seq_indent + 1;
            _.skipHorizontalWs();

            if (_.pos >= _.src.len || _.src[_.pos] == '\n' || _.src[_.pos] == '\r' || _.src[_.pos] == '#') {
                _.skipLineComment();
                if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                    _.skipToNextLine();
                }
                _.skipBlankAndCommentLines();
                if (_.pos < _.src.len && !_.isDocStartAtLine() && !_.isDocEndAtLine() && _.currentLineIndent() > seq_indent) {
                    const next_ind = _.currentLineIndent();
                    const child = try _.parseBlockFromCurrent(next_ind);
                    try items.append(_.allocator, child);
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
                const elem = try _.parseNode(item_col, false);
                try items.append(_.allocator, elem);
                _.skipHorizontalWs();
                _.skipLineComment();
                if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                    _.skipToNextLine();
                }
            }
        }

        return ConfValue.makeArray(try items.toOwnedSlice(_.allocator));
    }

    fn mergeObjectInto(fields: *YamlFieldList, src_obj: ConfValue, allocator: std.mem.Allocator): ! {
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
                try fields.append(allocator, ConfField{
                    key: try allocator.dupe(u8, sf.key),
                    value: try sf.value.clone(allocator),
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

    fn upsertField(fields: *YamlFieldList, key: []const u8, val: ConfValue, allocator: std.mem.Allocator): ! {
        for (*ef of fields.items) {
            if (std.mem.eql(u8, ef.key, key)) {
                ef.value = val;
                return;
            }
        }
        try fields.append(allocator, ConfField{ key: key, value: val });
    }

    fn parseBlockMapping(*_, map_indent: usize): ConfValue! {
        let fields = YamlFieldList.empty;
        let merge_sources = YamlValueList.empty;

        while (true) {
            _.skipBlankAndCommentLines();
            if (_.pos >= _.src.len || _.isDocStartAtLine() || _.isDocEndAtLine()) break;

            const line_ind = _.currentLineIndent();
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
                    const next_ind = _.currentLineIndent();
                    if (next_ind > map_indent) {
                        val = try _.parseBlockFromCurrent(next_ind);
                    } else if (next_ind == map_indent && isSeqMarkerAt(_.src, _.pos + next_ind)) {
                        _.pos += next_ind;
                        val = try _.parseBlockSequence(next_ind);
                    }
                }
            } else {
                val = try _.parseNode(map_indent, false);
                _.skipHorizontalWs();
                _.skipLineComment();
                if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                    _.skipToNextLine();
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
            try YamlParser.mergeObjectInto(&fields, ms, _.allocator);
        }
        merge_sources.deinit(_.allocator);

        return ConfValue.makeObject(try fields.toOwnedSlice(_.allocator));
    }

    fn parseBlockScalar(*_, parent_indent: usize): ConfValue! {
        const style = _.src[_.pos]; // '|' or '>'
        _.pos += 1;

        let chomp: u8 = 0; // 0=clip, 1=strip(-), 2=keep(+)
        let explicit_indent: usize = 0;

        while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r' && _.src[_.pos] != '#') {
            const c = _.src[_.pos];
            if (c == '-') {
                chomp = 1;
            } else if (c == '+') {
                chomp = 2;
            } else if (c >= '1' && c <= '9') {
                explicit_indent = (c - '0') as usize;
            } else if (c == ' ' || c == '\t') {
                // whitespace before comment
            } else {
                break;
            }
            _.pos += 1;
        }
        _.skipLineComment();
        if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
            _.skipToNextLine();
        }

        // Determine block content indentation
        let content_indent: usize = if (explicit_indent > 0) (parent_indent + explicit_indent) else 0;
        let scan_pos = _.pos;
        if (content_indent == 0) {
            while (scan_pos < _.src.len) {
                let sp: usize = 0;
                while (scan_pos + sp < _.src.len && _.src[scan_pos + sp] == ' ') sp += 1;
                if (scan_pos + sp >= _.src.len || _.src[scan_pos + sp] == '\n' || _.src[scan_pos + sp] == '\r') {
                    // Blank line
                    while (scan_pos < _.src.len && _.src[scan_pos] != '\n') scan_pos += 1;
                    if (scan_pos < _.src.len) scan_pos += 1;
                    continue;
                }
                content_indent = sp;
                break;
            }
            if (content_indent <= parent_indent && parent_indent > 0) {
                // Empty block scalar
                if (chomp == 2) {
                    // keep trailing newlines only if blank lines were present
                }
                return ConfValue.makeString("");
            }
        }

        let lines = YamlStrList.empty;
        defer lines.deinit(_.allocator);
        let trailing_blanks: usize = 0;

        while (_.pos < _.src.len) {
            if (_.isDocStartAtLine() || _.isDocEndAtLine()) break;
            let sp: usize = 0;
            while (_.pos + sp < _.src.len && _.src[_.pos + sp] == ' ') sp += 1;
            const after_sp = _.pos + sp;
            const is_blank = (after_sp >= _.src.len || _.src[after_sp] == '\n' || _.src[after_sp] == '\r');
            if (!is_blank && sp < content_indent) {
                break;
            }

            let line_end = after_sp;
            while (line_end < _.src.len && _.src[line_end] != '\n' && _.src[line_end] != '\r') {
                line_end += 1;
            }

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

        // Trim trailing blank lines based on chomp mode
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
            // Folded '>': blank lines produce one '\n' each; non-blank adjacent lines join with ' ' unless indented
            let i: usize = 0;
            let prev_was_indented = false;
            while (i < content_line_count) : (i += 1) {
                const ln = lines.items[i];
                const is_empty = (ln.len == 0);
                const is_indented = (!is_empty && (ln[0] == ' ' || ln[0] == '\t'));
                if (is_empty) {
                    if (i > 0) try out.append(_.allocator, '\n');
                    prev_was_indented = false;
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

    fn skipFlowWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
            } else if (c == '#') {
                _.skipToNextLine();
            } else {
                break;
            }
        }
    }

    fn parseFlowSeq(*_): ConfValue! {
        _.pos += 1; // '['
        let items = YamlValueList.empty;
        while (true) {
            _.skipFlowWs();
            if (_.pos >= _.src.len) return error.UnexpectedEof;
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            const elem = try _.parseNode(0, true);
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

            let key: []const u8 = "";
            if (_.src[_.pos] == '"') {
                key = try _.parseDoubleQuoted();
            } else if (_.src[_.pos] == '\'') {
                key = try _.parseSingleQuoted();
            } else {
                const start = _.pos;
                while (_.pos < _.src.len && _.src[_.pos] != ':' && _.src[_.pos] != ',' && _.src[_.pos] != '}' && _.src[_.pos] != '\n') {
                    _.pos += 1;
                }
                key = std.mem.trim(u8, _.src[start.._.pos], " \t");
            }

            _.skipFlowWs();
            let val = ConfValue.makeNull();
            if (_.pos < _.src.len && _.src[_.pos] == ':') {
                _.pos += 1;
                _.skipFlowWs();
                if (_.pos < _.src.len && _.src[_.pos] != ',' && _.src[_.pos] != '}') {
                    val = try _.parseNode(0, true);
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
            try YamlParser.mergeObjectInto(&fields, ms, _.allocator);
        }
        merge_sources.deinit(_.allocator);
        return ConfValue.makeObject(try fields.toOwnedSlice(_.allocator));
    }

    fn parseSingleQuoted(*_): ([]const u8)! {
        _.pos += 1; // opening '\''
        let out = YamlByteList.empty;
        while (_.pos < _.src.len) {
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
            try out.append(_.allocator, c);
            _.pos += 1;
        }
        return error.UnexpectedEof;
    }

    fn parseHexDigits(src: []const u8): u32? {
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

    fn appendUtf8(out: *YamlByteList, allocator: std.mem.Allocator, cp: u32): ! {
        if (cp > 0x10FFFF) return error.InvalidEscape;
        let buf: [4]u8 = undefined;
        const n = std.unicode.utf8Encode(@intCast(cp), &buf) catch return error.InvalidEscape;
        try out.appendSlice(allocator, buf[0..n]);
    }

    fn parseDoubleQuoted(*_): ([]const u8)! {
        _.pos += 1; // opening '"'
        let out = YamlByteList.empty;
        while (_.pos < _.src.len) {
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
                    't' => try out.append(_.allocator, '\t'),
                    'n' => try out.append(_.allocator, '\n'),
                    'v' => try out.append(_.allocator, 0x0B),
                    'f' => try out.append(_.allocator, 0x0C),
                    'r' => try out.append(_.allocator, '\r'),
                    'e' => try out.append(_.allocator, 0x1B),
                    ' ' => try out.append(_.allocator, ' '),
                    '"' => try out.append(_.allocator, '"'),
                    '/' => try out.append(_.allocator, '/'),
                    '\\' => try out.append(_.allocator, '\\'),
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
            try out.append(_.allocator, c);
            _.pos += 1;
        }
        return error.UnexpectedEof;
    }

    fn scanPlainScalar(*_, in_flow: bool): []const u8 {
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\n' || c == '\r') break;
            if (c == '#' && _.pos > start && (_.src[_.pos - 1] == ' ' || _.src[_.pos - 1] == '\t')) break;
            if (in_flow && (c == ',' || c == ']' || c == '}')) break;
            _.pos += 1;
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

    fn writeIndent(out: *YamlByteList, allocator: std.mem.Allocator, indent: usize): ! {
        let k: usize = 0;
        while (k < indent) : (k += 1) {
            try out.append(allocator, ' ');
        }
    }

    fn needsQuotes(s: []const u8): bool {
        if (s.len == 0) return true;
        if (std.mem.eql(u8, s, "true") || std.mem.eql(u8, s, "false") || std.mem.eql(u8, s, "null") || std.mem.eql(u8, s, "~")) return true;
        if (s[0] == ' ' || s[s.len - 1] == ' ' || s[0] == '-' || s[0] == '#' || s[0] == '&' || s[0] == '*' || s[0] == '!' || s[0] == '[' || s[0] == '{') return true;
        for (c of s) {
            if (c == ':' || c == '#' || c == '\n' || c == '\r' || c == '"' || c == '\t') return true;
        }
        if (parseYamlInt(s) != null || parseYamlFloat(s) != null) return true;
        return false;
    }

    fn writeYamlString(out: *YamlByteList, allocator: std.mem.Allocator, s: []const u8): ! {
        if (!YamlParser.needsQuotes(s)) {
            try out.appendSlice(allocator, s);
            return;
        }
        try out.append(allocator, '"');
        for (c of s) {
            switch (c) {
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

    fn writeValue(out: *YamlByteList, allocator: std.mem.Allocator, val: ConfValue, indent: usize): ! {
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
        if (c == '.' || c == 'e' || c == 'E') {
            has_dot_or_exp = true;
            break;
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
