[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/treesitter

Tree-sitter incremental parsing library (v0.25.10) native C runtime for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/treesitter` |
| Version | `0.25.10` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/treesitter
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `TreeSitterError` | `AllocFailed, SetLanguageFailed, ParseFailed, CopyFailed, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TSNode` | `class` | Fields: `context: [4]u32, id: *auto!?, tree: *auto!?` |
| `TsNode` | `class` | Fields: `raw: TSNode` · Methods: `typeName, startByte, endByte, childCount, namedChildCount, isNull, +2 more` |
| `TsTree` | `class` | Fields: `tree_ptr: *auto!?` · Methods: `rootNode, copy, raw, __cp_deinit` |
| `TsParser` | `class` | Fields: `parser_ptr: *auto!?` · Methods: `init, setLanguage, parseString, reset, setTimeoutMicros, timeoutMicros, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ts_parser_new` | `(): *auto!?` | Invokes `ts_parser_new` and returns `*auto!?`. |
| `ts_parser_delete` | `(parser: *auto!?)` | Executes `ts_parser_delete` in-place. |
| `ts_parser_set_language` | `(parser: *auto!?, language: *auto!?): bool` | Invokes `ts_parser_set_language` and returns `bool`. |
| `ts_parser_parse_string` | `(parser: *auto!?, old_tree: *auto!?, input: [*]&u8!, length: u32): *auto!?` | Invokes `ts_parser_parse_string` and returns `*auto!?`. |
| `ts_parser_reset` | `(parser: *auto!?)` | Executes `ts_parser_reset` in-place. |
| `ts_parser_set_timeout_micros` | `(parser: *auto!?, timeout_micros: u64)` | Executes `ts_parser_set_timeout_micros` in-place. |
| `ts_parser_timeout_micros` | `(parser: *auto!?): u64` | Invokes `ts_parser_timeout_micros` and returns `u64`. |
| `ts_tree_delete` | `(tree: *auto!?)` | Executes `ts_tree_delete` in-place. |
| `ts_tree_root_node` | `(tree: *auto!?): TSNode` | Invokes `ts_tree_root_node` and returns `TSNode`. |
| `ts_tree_copy` | `(tree: *auto!?): *auto!?` | Invokes `ts_tree_copy` and returns `*auto!?`. |
| `ts_node_type` | `(node: TSNode): [*:0]&u8!` | Invokes `ts_node_type` and returns `[*:0]&u8!`. |
| `ts_node_start_byte` | `(node: TSNode): u32` | Invokes `ts_node_start_byte` and returns `u32`. |
| `ts_node_end_byte` | `(node: TSNode): u32` | Invokes `ts_node_end_byte` and returns `u32`. |
| `ts_node_child_count` | `(node: TSNode): u32` | Invokes `ts_node_child_count` and returns `u32`. |
| `ts_node_named_child_count` | `(node: TSNode): u32` | Invokes `ts_node_named_child_count` and returns `u32`. |
| `ts_node_is_null` | `(node: TSNode): bool` | Invokes `ts_node_is_null` and returns `bool`. |
| `ts_node_has_error` | `(node: TSNode): bool` | Invokes `ts_node_has_error` and returns `bool`. |
| `ts_node_string` | `(node: TSNode): [*:0]u8!?` | Invokes `ts_node_string` and returns `[*:0]u8!?`. |
| `ts_language_version` | `(lang: *auto!?): u32` | Invokes `ts_language_version` and returns `u32`. |
| `ts_language_abi_version` | `(lang: *auto!?): u32` | Invokes `ts_language_abi_version` and returns `u32`. |
| `ts_language_symbol_count` | `(lang: *auto!?): u32` | Invokes `ts_language_symbol_count` and returns `u32`. |
| `abiVersion` | `(): u32` | Invokes `abiVersion` and returns `u32`. |
| `minCompatibleAbiVersion` | `(): u32` | Invokes `minCompatibleAbiVersion` and returns `u32`. |

## Minimal Example

```cp
import { ts_parser_new } from "@cap/treesitter";

pub fn main() {
    let result = ts_parser_new();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
