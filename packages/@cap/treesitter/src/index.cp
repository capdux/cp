// @cap/treesitter — Tree-sitter incremental parsing library (v0.25.10) for CP
// License: MIT (Upstream: tree-sitter/tree-sitter)

import std from "std";

pub @c struct TSNode {
    context: [4]u32 = [0, 0, 0, 0];
    id: *!? = null;
    tree: *!? = null;
}

pub @c {
    fn ts_parser_new(): *!?;
    fn ts_parser_delete(parser: *!?);
    fn ts_parser_set_language(parser: *!?, language: *!?): bool;
    fn ts_parser_parse_string(parser: *!?, old_tree: *!?, input: [*]const u8!, length: u32): *!?;
    fn ts_parser_reset(parser: *!?);
    fn ts_parser_set_timeout_micros(parser: *!?, timeout_micros: u64);
    fn ts_parser_timeout_micros(parser: *!?): u64;

    fn ts_tree_delete(tree: *!?);
    fn ts_tree_root_node(tree: *!?): TSNode;
    fn ts_tree_copy(tree: *!?): *!?;

    fn ts_node_type(node: TSNode): [*:0]const u8;
    fn ts_node_start_byte(node: TSNode): u32;
    fn ts_node_end_byte(node: TSNode): u32;
    fn ts_node_child_count(node: TSNode): u32;
    fn ts_node_named_child_count(node: TSNode): u32;
    fn ts_node_is_null(node: TSNode): bool;
    fn ts_node_has_error(node: TSNode): bool;
    fn ts_node_string(node: TSNode): [*:0]u8?;

    fn ts_language_version(lang: *!?): u32;
    fn ts_language_abi_version(lang: *!?): u32;
    fn ts_language_symbol_count(lang: *!?): u32;
}

pub const TREE_SITTER_LANGUAGE_VERSION: u32 = 15;
pub const TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION: u32 = 13;

/// Returns the latest language ABI version supported by this tree-sitter runtime.
pub fn abiVersion(): u32 {
    return TREE_SITTER_LANGUAGE_VERSION;
}

/// Returns the minimum compatible language ABI version supported by this runtime.
pub fn minCompatibleAbiVersion(): u32 {
    return TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION;
}

pub err TreeSitterError {
    AllocFailed,
    SetLanguageFailed,
    ParseFailed,
    CopyFailed,
    OutOfMemory,
}

/// Non-owning value wrapper around a tree-sitter `TSNode`.
pub struct TsNode {
    raw: TSNode,

    pub fn typeName(): string {
        unsafe {
            return std.mem.span(ts_node_type(_.raw));
        }
    }

    pub fn startByte(): u32 {
        unsafe { return ts_node_start_byte(_.raw); }
    }

    pub fn endByte(): u32 {
        unsafe { return ts_node_end_byte(_.raw); }
    }

    pub fn childCount(): u32 {
        unsafe { return ts_node_child_count(_.raw); }
    }

    pub fn namedChildCount(): u32 {
        unsafe { return ts_node_named_child_count(_.raw); }
    }

    pub fn isNull(): bool {
        unsafe { return ts_node_is_null(_.raw); }
    }

    pub fn hasError(): bool {
        unsafe { return ts_node_has_error(_.raw); }
    }

    pub fn toSexp(allocator: std.mem.Allocator): []u8! {
        let c_str: [*:0]u8? = null;
        unsafe { c_str = ts_node_string(_.raw); }
        if (ptr := c_str) {
            defer { unsafe { std.c.free(ptr); } }
            const slice = std.mem.span(ptr);
            const owned = allocator.alloc(u8, slice.len) catch return TreeSitterError.OutOfMemory;
            @memcpy(owned, slice);
            return owned;
        }
        return TreeSitterError.AllocFailed;
    }
}

/// Owned RAII syntax tree backed by `TSTree`. Automatically freed on scope exit.
pub struct TsTree {
    tree_ptr: *!? = null,

    pub fn rootNode(): TsNode {
        if (t := _.tree_ptr) {
            unsafe {
                return TsNode{ .raw = ts_tree_root_node(t) };
            }
        }
        return TsNode{ .raw = TSNode{} };
    }

    pub fn copy(): TsTree! {
        if (t := _.tree_ptr) {
            let cloned: *!? = null;
            unsafe { cloned = ts_tree_copy(t); }
            if (cloned == null) return TreeSitterError.CopyFailed;
            return TsTree{ .tree_ptr = cloned };
        }
        return TreeSitterError.CopyFailed;
    }

    pub fn raw(): *!? {
        return _.tree_ptr;
    }

    ~init() {
        if (t := _.tree_ptr) {
            _.tree_ptr = null;
            unsafe { ts_tree_delete(t); }
        }
    }
}

/// Owned RAII parser backed by `TSParser`. Automatically freed on scope exit.
pub struct TsParser {
    parser_ptr: *!? = null,

    pub static fn init(): TsParser! {
        let p: *!? = null;
        unsafe { p = ts_parser_new(); }
        if (p == null) return TreeSitterError.AllocFailed;
        return TsParser{ .parser_ptr = p };
    }

    pub fn setLanguage(language: *!?): void! {
        if (p := _.parser_ptr) {
            let ok = false;
            unsafe { ok = ts_parser_set_language(p, language); }
            if (!ok) return TreeSitterError.SetLanguageFailed;
            return;
        }
        return TreeSitterError.SetLanguageFailed;
    }

    pub fn parseString(source: []const u8, old_tree: *!?): TsTree! {
        if (p := _.parser_ptr) {
            let tree: *!? = null;
            const len_u32: u32 = @intCast(source.len);
            unsafe {
                tree = ts_parser_parse_string(p, old_tree, source.ptr, len_u32);
            }
            if (tree == null) return TreeSitterError.ParseFailed;
            return TsTree{ .tree_ptr = tree };
        }
        return TreeSitterError.ParseFailed;
    }

    pub fn reset() {
        if (p := _.parser_ptr) {
            unsafe { ts_parser_reset(p); }
        }
    }

    pub fn setTimeoutMicros(timeout_micros: u64) {
        if (p := _.parser_ptr) {
            unsafe { ts_parser_set_timeout_micros(p, timeout_micros); }
        }
    }

    pub fn timeoutMicros(): u64 {
        if (p := _.parser_ptr) {
            unsafe { return ts_parser_timeout_micros(p); }
        }
        return 0;
    }

    pub fn raw(): *!? {
        return _.parser_ptr;
    }

    ~init() {
        if (p := _.parser_ptr) {
            _.parser_ptr = null;
            unsafe { ts_parser_delete(p); }
        }
    }
}
