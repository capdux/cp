[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/treesitter

> 版本：`0.25.10` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Tree-sitter incremental parsing library (v0.25.10) native C runtime for CP

## 安装与导入

```bash
cap add @cap/treesitter
```

```cp
import { ts_parser_new } from "@cap/treesitter";
```

## 核心导出 API

- `export fn ts_parser_new(): *auto!?;`
- `export fn ts_parser_delete(parser: *auto!?);`
- `export fn ts_parser_set_language(parser: *auto!?, language: *auto!?): bool;`
- `export fn ts_parser_parse_string(parser: *auto!?, old_tree: *auto!?, input: [*]&u8!, length: u32): *auto!?;`
- `export fn ts_parser_reset(parser: *auto!?);`
- `export fn ts_parser_set_timeout_micros(parser: *auto!?, timeout_micros: u64);`
- `export fn ts_parser_timeout_micros(parser: *auto!?): u64;`
- `export fn ts_tree_delete(tree: *auto!?);`
- `export fn ts_tree_root_node(tree: *auto!?): TSNode;`
- `export fn ts_tree_copy(tree: *auto!?): *auto!?;`
- `export fn ts_node_type(node: TSNode): [*:0]&u8!;`
- `export fn ts_node_start_byte(node: TSNode): u32;`
- `export fn ts_node_end_byte(node: TSNode): u32;`
- `export fn ts_node_child_count(node: TSNode): u32;`
- `export fn ts_node_named_child_count(node: TSNode): u32;`
- `export fn ts_node_is_null(node: TSNode): bool;`
- `export fn ts_node_has_error(node: TSNode): bool;`
- `export fn ts_node_string(node: TSNode): [*:0]u8!?;`
- `export fn ts_language_version(lang: *auto!?): u32;`
- `export fn ts_language_abi_version(lang: *auto!?): u32;`
- `export fn ts_language_symbol_count(lang: *auto!?): u32;`
- `export fn abiVersion(): u32;`
- `export fn minCompatibleAbiVersion(): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`tree-sitter@0.25.10`)，许可证：MIT
