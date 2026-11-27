[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/treesitter

Tree-sitter incremental parsing library (v0.25.10) native C runtime for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/treesitter` |
| 版本 | `0.25.10` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

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

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `TreeSitterError` | `AllocFailed, SetLanguageFailed, ParseFailed, CopyFailed, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TSNode` | `class` | 字段: `context: [4]u32, id: *auto!?, tree: *auto!?` |
| `TsNode` | `class` | 字段: `raw: TSNode` · 方法: `typeName, startByte, endByte, childCount, namedChildCount, isNull, +2 项` |
| `TsTree` | `class` | 字段: `tree_ptr: *auto!?` · 方法: `rootNode, copy, raw, __cp_deinit` |
| `TsParser` | `class` | 字段: `parser_ptr: *auto!?` · 方法: `init, setLanguage, parseString, reset, setTimeoutMicros, timeoutMicros, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ts_parser_new` | `(): *auto!?` | 调用 `ts_parser_new` 并返回 `*auto!?`。 |
| `ts_parser_delete` | `(parser: *auto!?)` | 就地执行 `ts_parser_delete` 操作。 |
| `ts_parser_set_language` | `(parser: *auto!?, language: *auto!?): bool` | 调用 `ts_parser_set_language` 并返回 `bool`。 |
| `ts_parser_parse_string` | `(parser: *auto!?, old_tree: *auto!?, input: [*]&u8!, length: u32): *auto!?` | 调用 `ts_parser_parse_string` 并返回 `*auto!?`。 |
| `ts_parser_reset` | `(parser: *auto!?)` | 就地执行 `ts_parser_reset` 操作。 |
| `ts_parser_set_timeout_micros` | `(parser: *auto!?, timeout_micros: u64)` | 就地执行 `ts_parser_set_timeout_micros` 操作。 |
| `ts_parser_timeout_micros` | `(parser: *auto!?): u64` | 调用 `ts_parser_timeout_micros` 并返回 `u64`。 |
| `ts_tree_delete` | `(tree: *auto!?)` | 就地执行 `ts_tree_delete` 操作。 |
| `ts_tree_root_node` | `(tree: *auto!?): TSNode` | 调用 `ts_tree_root_node` 并返回 `TSNode`。 |
| `ts_tree_copy` | `(tree: *auto!?): *auto!?` | 调用 `ts_tree_copy` 并返回 `*auto!?`。 |
| `ts_node_type` | `(node: TSNode): [*:0]&u8!` | 调用 `ts_node_type` 并返回 `[*:0]&u8!`。 |
| `ts_node_start_byte` | `(node: TSNode): u32` | 调用 `ts_node_start_byte` 并返回 `u32`。 |
| `ts_node_end_byte` | `(node: TSNode): u32` | 调用 `ts_node_end_byte` 并返回 `u32`。 |
| `ts_node_child_count` | `(node: TSNode): u32` | 调用 `ts_node_child_count` 并返回 `u32`。 |
| `ts_node_named_child_count` | `(node: TSNode): u32` | 调用 `ts_node_named_child_count` 并返回 `u32`。 |
| `ts_node_is_null` | `(node: TSNode): bool` | 调用 `ts_node_is_null` 并返回 `bool`。 |
| `ts_node_has_error` | `(node: TSNode): bool` | 调用 `ts_node_has_error` 并返回 `bool`。 |
| `ts_node_string` | `(node: TSNode): [*:0]u8!?` | 调用 `ts_node_string` 并返回 `[*:0]u8!?`。 |
| `ts_language_version` | `(lang: *auto!?): u32` | 调用 `ts_language_version` 并返回 `u32`。 |
| `ts_language_abi_version` | `(lang: *auto!?): u32` | 调用 `ts_language_abi_version` 并返回 `u32`。 |
| `ts_language_symbol_count` | `(lang: *auto!?): u32` | 调用 `ts_language_symbol_count` 并返回 `u32`。 |
| `abiVersion` | `(): u32` | 调用 `abiVersion` 并返回 `u32`。 |
| `minCompatibleAbiVersion` | `(): u32` | 调用 `minCompatibleAbiVersion` 并返回 `u32`。 |

## 最小调用示例

```cp
import { ts_parser_new } from "@cap/treesitter";

pub fn main() {
    let result = ts_parser_new();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
