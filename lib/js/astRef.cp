// Source: Absorbed from /data/dev/bun/src/js_parser/ast/base.cp | Synced: 2026-11-20T11:56:02Z
// ============================================================================
// lib/js/astRef.cp — Zero-Alloc 64-bit Packed AST `Ref`, Typed `SourceIndex`
// & Parallel-Parser 2D Symbol Map Merger
// ============================================================================
// Absorbed from Bun `js_parser/ast/base.cp` (esbuild/Bun parallel AST Ref
// architecture). Packs `(inner_index: u31, tag: u2, source_index: u31)` into
// a single `u64` where `NONE == 0`, with SplitMix64 hashing, canonical
// formatting (`Ref[inner=..., src=..., .tag]` & JSON `[src,inner]`), and an
// external-slice-driven O(1) 2D `JsMergedSymbolMap` for merging per-file
// parallel parser symbol tables.

pub const JS_SOURCE_INDEX_INVALID: u32 = 0xFFFFFFFF;
pub const JS_SOURCE_INDEX_RUNTIME: u32 = 0;
pub const JS_SOURCE_INDEX_BAKE_SERVER_DATA: u32 = 1;
pub const JS_SOURCE_INDEX_BAKE_CLIENT_DATA: u32 = 2;

pub const JS_AST_REF_MAX_INDEX: u32 = 0x7FFFFFFF;

pub err JsAstRefError {
    IndexOutOfRange,
    InvalidRef,
    SourceOutOfBounds,
    TableFull,
    BufferTooSmall,
}

fn appendByte(out: []u8, pos: usize, ch: u8): JsAstRefError!usize {
    if (pos >= out.len) return JsAstRefError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): JsAstRefError!usize {
    if (src.len > out.len - pos) return JsAstRefError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeU32(out: []u8, pos: usize, val_in: u32): JsAstRefError!usize {
    let tmp: [12]u8 = [0; 12];
    let n: usize = 0;
    let v = val_in;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
    }
    if (n > out.len - pos) return JsAstRefError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// Strongly-typed 32-bit source/part index matching Bun `ast.Index`.
pub struct JsSourceIndex {
    value: u32 = JS_SOURCE_INDEX_INVALID,

    static fn invalid(): JsSourceIndex {
        return JsSourceIndex{ value: JS_SOURCE_INDEX_INVALID };
    }

    static fn runtime(): JsSourceIndex {
        return JsSourceIndex{ value: JS_SOURCE_INDEX_RUNTIME };
    }

    static fn fromU32(v: u32): JsSourceIndex {
        return JsSourceIndex{ value: v };
    }

    fn get(): u32 {
        return _.value;
    }

    fn set(*_, v: u32) {
        _.value = v;
    }

    fn isValid(): bool {
        return _.value != JS_SOURCE_INDEX_INVALID;
    }

    fn isInvalid(): bool {
        return _.value == JS_SOURCE_INDEX_INVALID;
    }

    fn isRuntime(): bool {
        return _.value == JS_SOURCE_INDEX_RUNTIME;
    }

    fn eql(other: JsSourceIndex): bool {
        return _.value == other.value;
    }
}

/// 2-bit tag discriminant matching Bun `ast.RefTag`.
pub enum JsAstRefTag: u8 {
    invalid = 0,
    allocated_name = 1,
    source_contents_slice = 2,
    symbol = 3,

    fn asStr(): []const u8 {
        if (_ == JsAstRefTag.invalid) return "invalid";
        if (_ == JsAstRefTag.allocated_name) return "allocated_name";
        if (_ == JsAstRefTag.source_contents_slice) return "source_contents_slice";
        return "symbol";
    }
}

/// 64-bit packed AST symbol reference `(inner_index: u31, tag: u2, source_index: u31)`.
/// Bit layout: `[0..31) inner_index | [31..33) tag | [33..64) source_index`.
/// `JsAstRef.none()` has bit representation `0`.
pub struct JsAstRef {
    raw: u64 = 0,

    static fn none(): JsAstRef {
        return JsAstRef{ raw: 0 };
    }

    static fn fromU64(bits: u64): JsAstRef {
        return JsAstRef{ raw: bits };
    }

    static fn pack(inner_index: u32, source_index: u32, ref_tag: JsAstRefTag): JsAstRefError!JsAstRef {
        if (inner_index > JS_AST_REF_MAX_INDEX || source_index > JS_AST_REF_MAX_INDEX) {
            return JsAstRefError.IndexOutOfRange;
        }
        const t: u64 = @as(u64, @intFromEnum(ref_tag)) & 0x3;
        const packed_val: u64 = (@as(u64, inner_index) & 0x7FFFFFFF)
            | (t << 31)
            | ((@as(u64, source_index) & 0x7FFFFFFF) << 33);
        return JsAstRef{ raw: packed_val };
    }

    /// Mirrors Bun `Ref.init(inner_index, source_index, is_source_contents_slice)`.
    static fn init(inner_index: u32, source_index: u32, is_source_contents_slice: bool): JsAstRefError!JsAstRef {
        const ref_tag = if (is_source_contents_slice)
            JsAstRefTag.source_contents_slice
        else
            JsAstRefTag.allocated_name;
        return JsAstRef.pack(inner_index, source_index, ref_tag);
    }

    /// Creates a `.symbol`-tagged reference for a parser/bundler symbol slot.
    static fn initSymbol(inner_index: u32, source_index: u32): JsAstRefError!JsAstRef {
        return JsAstRef.pack(inner_index, source_index, JsAstRefTag.symbol);
    }

    fn asU64(): u64 {
        return _.raw;
    }

    fn innerIndex(): u32 {
        return @as(u32, @truncate(_.raw & 0x7FFFFFFF));
    }

    fn tag(): JsAstRefTag {
        const t: u8 = @as(u8, @truncate((_.raw >> 31) & 0x3));
        if (t == 1) return JsAstRefTag.allocated_name;
        if (t == 2) return JsAstRefTag.source_contents_slice;
        if (t == 3) return JsAstRefTag.symbol;
        return JsAstRefTag.invalid;
    }

    fn sourceIndex(): u32 {
        return @as(u32, @truncate((_.raw >> 33) & 0x7FFFFFFF));
    }

    fn isEmpty(): bool {
        return _.raw == 0;
    }

    fn isValid(): bool {
        return _.tag() != JsAstRefTag.invalid;
    }

    fn isSymbol(): bool {
        return _.tag() == JsAstRefTag.symbol;
    }

    fn isSourceContentsSlice(): bool {
        return _.tag() == JsAstRefTag.source_contents_slice;
    }

    fn isAllocatedName(): bool {
        return _.tag() == JsAstRefTag.allocated_name;
    }

    fn isSourceIndexNull(): bool {
        return _.sourceIndex() == JS_AST_REF_MAX_INDEX;
    }

    /// 64-bit SplitMix64 avalanche hash of the packed `Ref`.
    fn hash64(): u64 {
        let z = _.raw +% 0x9E3779B97F4A7C15;
        z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
        z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
        return z ^ (z >> 31);
    }

    fn hash32(): u32 {
        return @as(u32, @truncate(_.hash64()));
    }

    fn eql(other: JsAstRef): bool {
        return _.raw == other.raw;
    }

    /// Deterministic ordering by `(source_index, inner_index, tag)`.
    fn compare(other: JsAstRef): i32 {
        const s1 = _.sourceIndex();
        const s2 = other.sourceIndex();
        if (s1 < s2) return -1;
        if (s1 > s2) return 1;
        const idx1 = _.innerIndex();
        const idx2 = other.innerIndex();
        if (idx1 < idx2) return -1;
        if (idx1 > idx2) return 1;
        const t1 = @intFromEnum(_.tag());
        const t2 = @intFromEnum(other.tag());
        if (t1 < t2) return -1;
        if (t1 > t2) return 1;
        return 0;
    }

    /// Formats `"Ref[inner=<d>, src=<d>, .<tag>]"` into `out` (matches Bun `Ref.format`).
    fn format(out: []u8): JsAstRefError!usize {
        let pos = appendSlice(out, 0, "Ref[inner=")?;
        pos = writeU32(out, pos, _.innerIndex())?;
        pos = appendSlice(out, pos, ", src=")?;
        pos = writeU32(out, pos, _.sourceIndex())?;
        pos = appendSlice(out, pos, ", .")?;
        pos = appendSlice(out, pos, _.tag().asStr())?;
        return appendByte(out, pos, ']');
    }

    /// Formats JSON pair `"[<src>,<inner>]"` into `out` (matches Bun `Ref.jsonStringify`).
    fn formatJsonPair(out: []u8): JsAstRefError!usize {
        let pos = appendByte(out, 0, '[')?;
        pos = writeU32(out, pos, _.sourceIndex())?;
        pos = appendByte(out, pos, ',')?;
        pos = writeU32(out, pos, _.innerIndex())?;
        return appendByte(out, pos, ']');
    }
}

/// Per-source slice range inside a flattened `JsMergedSymbolMap`.
pub struct JsSourceTableSpan {
    offset: usize = 0,
    len: usize = 0,
    registered: bool = false,

    static fn empty(): JsSourceTableSpan {
        return JsSourceTableSpan{};
    }
}

/// External-slice-driven O(1) 2D symbol map merger for parallel-parsed files.
/// Maps any valid `JsAstRef(source_index, inner_index)` to a dense flat slot.
pub struct JsMergedSymbolMap {
    spans: []JsSourceTableSpan,
    entries: []u32,
    entry_len: usize = 0,

    static fn init(spans: []JsSourceTableSpan, entries: []u32): JsMergedSymbolMap {
        let i: usize = 0;
        while (i < spans.len) {
            spans[i] = JsSourceTableSpan.empty();
            i += 1;
        }
        return JsMergedSymbolMap{
            spans: spans,
            entries: entries,
            entry_len: 0,
        };
    }

    fn totalSymbols(): usize {
        return _.entry_len;
    }

    /// Reserves `symbol_count` contiguous slots for `source_index`, initializing them to `default_val`.
    fn registerSource(*_, source_index: u32, symbol_count: usize, default_val: u32): JsAstRefError!void {
        const si: usize = @as(usize, source_index);
        if (si >= _.spans.len) return JsAstRefError.SourceOutOfBounds;
        if (symbol_count > _.entries.len - _.entry_len) return JsAstRefError.TableFull;

        const base = _.entry_len;
        let i: usize = 0;
        while (i < symbol_count) {
            _.entries[base + i] = default_val;
            i += 1;
        }
        _.spans[si] = JsSourceTableSpan{
            offset: base,
            len: symbol_count,
            registered: true,
        };
        _.entry_len += symbol_count;
    }

    /// Resolves `ref` to its flat slot index in `entries` in O(1).
    fn resolveSlot(ref_val: JsAstRef): JsAstRefError!usize {
        if (!ref_val.isValid()) return JsAstRefError.InvalidRef;
        const si: usize = @as(usize, ref_val.sourceIndex());
        if (si >= _.spans.len || !_.spans[si].registered) {
            return JsAstRefError.SourceOutOfBounds;
        }
        const ii: usize = @as(usize, ref_val.innerIndex());
        if (ii >= _.spans[si].len) {
            return JsAstRefError.IndexOutOfRange;
        }
        return _.spans[si].offset + ii;
    }

    fn get(ref_val: JsAstRef): JsAstRefError!u32 {
        const slot = _.resolveSlot(ref_val)?;
        return _.entries[slot];
    }

    fn set(*_, ref_val: JsAstRef, value: u32): JsAstRefError!void {
        const slot = _.resolveSlot(ref_val)?;
        _.entries[slot] = value;
    }
}
