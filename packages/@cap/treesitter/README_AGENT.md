# @cap/treesitter — AI Agent Quick Ref

Tree-sitter incremental parsing library (v0.25.10) native C runtime for CP (`v0.25.10`, `MIT`)

## Import

```cp
import {
    TreeSitterError,
    TSNode,
    TsNode,
    TsTree,
    TsParser,
    ts_parser_new,
    ts_parser_delete,
    ts_parser_set_language,
} from "@cap/treesitter";
```

## Core Types & Constants

```cp
error TreeSitterError { AllocFailed, SetLanguageFailed, ParseFailed, CopyFailed, OutOfMemory }
class TSNode { context: [4]u32, id: *auto!?, tree: *auto!? }
class TsNode { raw: TSNode } // methods: typeName(...) +7
class TsTree { tree_ptr: *auto!? } // methods: rootNode(...) +3
class TsParser { parser_ptr: *auto!? } // methods: init(...) +7
```

## API Signatures

```cp
fn ts_parser_new(): *auto!?;
fn ts_parser_delete(parser: *auto!?);
fn ts_parser_set_language(parser: *auto!?, language: *auto!?): bool;
fn ts_parser_parse_string(parser: *auto!?, old_tree: *auto!?, input: [*]&u8!, length: u32): *auto!?;
fn ts_parser_reset(parser: *auto!?);
fn ts_parser_set_timeout_micros(parser: *auto!?, timeout_micros: u64);
fn ts_parser_timeout_micros(parser: *auto!?): u64;
fn ts_tree_delete(tree: *auto!?);
fn ts_tree_root_node(tree: *auto!?): TSNode;
fn ts_tree_copy(tree: *auto!?): *auto!?;
fn ts_node_type(node: TSNode): [*:0]&u8!;
fn ts_node_start_byte(node: TSNode): u32;
fn ts_node_end_byte(node: TSNode): u32;
fn ts_node_child_count(node: TSNode): u32;
fn ts_node_named_child_count(node: TSNode): u32;
fn ts_node_is_null(node: TSNode): bool;
fn ts_node_has_error(node: TSNode): bool;
fn ts_node_string(node: TSNode): [*:0]u8!?;
fn ts_language_version(lang: *auto!?): u32;
fn ts_language_abi_version(lang: *auto!?): u32;
fn ts_language_symbol_count(lang: *auto!?): u32;
fn abiVersion(): u32;
fn minCompatibleAbiVersion(): u32;
```

## Minimal Usage

```cp
import { ts_parser_new } from "@cap/treesitter";

pub fn main() {
    let result = ts_parser_new();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/treesitter"`; install via `cap add @cap/treesitter`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
