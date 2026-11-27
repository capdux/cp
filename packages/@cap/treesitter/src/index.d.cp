// Type definitions for CP

import {  } from "std";
export class TSNode {
    context: [4]u32,
    id: *auto!?,
    tree: *auto!?,
}

export fn ts_parser_new(): *auto!?;
export fn ts_parser_delete(parser: *auto!?);
export fn ts_parser_set_language(parser: *auto!?, language: *auto!?): bool;
export fn ts_parser_parse_string(parser: *auto!?, old_tree: *auto!?, input: [*]&u8!, length: u32): *auto!?;
export fn ts_parser_reset(parser: *auto!?);
export fn ts_parser_set_timeout_micros(parser: *auto!?, timeout_micros: u64);
export fn ts_parser_timeout_micros(parser: *auto!?): u64;
export fn ts_tree_delete(tree: *auto!?);
export fn ts_tree_root_node(tree: *auto!?): TSNode;
export fn ts_tree_copy(tree: *auto!?): *auto!?;
export fn ts_node_type(node: TSNode): [*:0]&u8!;
export fn ts_node_start_byte(node: TSNode): u32;
export fn ts_node_end_byte(node: TSNode): u32;
export fn ts_node_child_count(node: TSNode): u32;
export fn ts_node_named_child_count(node: TSNode): u32;
export fn ts_node_is_null(node: TSNode): bool;
export fn ts_node_has_error(node: TSNode): bool;
export fn ts_node_string(node: TSNode): [*:0]u8!?;
export fn ts_language_version(lang: *auto!?): u32;
export fn ts_language_abi_version(lang: *auto!?): u32;
export fn ts_language_symbol_count(lang: *auto!?): u32;
export fn abiVersion(): u32;
export fn minCompatibleAbiVersion(): u32;
export error TreeSitterError { AllocFailed, SetLanguageFailed, ParseFailed, CopyFailed, OutOfMemory }

export class TsNode {
    raw: TSNode,
    fn typeName(): string;
    fn startByte(): u32;
    fn endByte(): u32;
    fn childCount(): u32;
    fn namedChildCount(): u32;
    fn isNull(): bool;
    fn hasError(): bool;
    fn toSexp(allocator: std.mem.Allocator): ![]u8;
}

export class TsTree {
    tree_ptr: *auto!?,
    fn rootNode(): TsNode;
    fn copy(): !TsTree;
    fn raw(): *auto!?;
    fn __cp_deinit(_: &mut TsTree);
}

export class TsParser {
    parser_ptr: *auto!?,
    fn init(): !TsParser;
    fn setLanguage(language: *auto!?): !void;
    fn parseString(source: &[]u8, old_tree: *auto!?): !TsTree;
    fn reset();
    fn setTimeoutMicros(timeout_micros: u64);
    fn timeoutMicros(): u64;
    fn raw(): *auto!?;
    fn __cp_deinit(_: &mut TsParser);
}

