[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# json-schema-traverse

Zero-allocation JSON Schema traversal engine with RFC 6901 JSON Pointer generation for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:json-schema-traverse` ([https://github.com/epoberezkin/json-schema-traverse](https://github.com/epoberezkin/json-schema-traverse))

| 元信息 | 值 |
| --- | --- |
| 包名 | `json-schema-traverse` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add json-schema-traverse
```

```cp
import {
    SchemaNodeKind,
    SchemaVisitEvent,
    JstvSpan,
    JstvTraverseState,
    jstvIsWs,
    jstvSkipWs,
    jstvScanString,
    jstvScanValue,
} from "json-schema-traverse";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SchemaNodeKind` | `Root, Property, Items, AdditionalProperties, AllOf, AnyOf, OneOf, Not ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SchemaVisitEvent` | `class` | 字段: `pointer_start: usize, pointer_end: usize, pointer_buf: [128]u8, pointer_len: usize, kind: SchemaNodeKind, schema_start: usize, +2 项` · 方法: `pointer, schemaSlice` |
| `JstvSpan` | `class` | 字段: `start: usize, end: usize, ok: bool` |
| `JstvTraverseState` | `class` | 字段: `src: string, out: []SchemaVisitEvent, written: usize, total: usize, target_ptr: string, find_mode: bool, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `jstvIsWs` | `(b: u8): bool` | 调用 `jstvIsWs` 并返回 `bool`。 |
| `jstvSkipWs` | `(src: string, start: usize): usize` | 调用 `jstvSkipWs` 并返回 `usize`。 |
| `jstvScanString` | `(src: string, start: usize): JstvSpan` | 调用 `jstvScanString` 并返回 `JstvSpan`。 |
| `jstvScanValue` | `(src: string, start: usize): JstvSpan` | 调用 `jstvScanValue` 并返回 `JstvSpan`。 |
| `jstvStrEq` | `(a: string, b: string): bool` | 调用 `jstvStrEq` 并返回 `bool`。 |
| `jstvCopyBuf` | `(src: [128]u8, len: usize): [128]u8` | 调用 `jstvCopyBuf` 并返回 `[128]u8`。 |
| `jstvAppendByte` | `(buf: &mut [128]u8, cur_len: usize, b: u8): usize` | 调用 `jstvAppendByte` 并返回 `usize`。 |
| `jstvAppendEscapedSegment` | `(buf: &mut [128]u8, cur_len: usize, seg: string): usize` | 调用 `jstvAppendEscapedSegment` 并返回 `usize`。 |
| `jstvAppendIndexSegment` | `(buf: &mut [128]u8, cur_len: usize, idx: usize): usize` | 调用 `jstvAppendIndexSegment` 并返回 `usize`。 |
| `jstvEmitEvent` | `(st: &mut JstvTraverseState, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, schema_start: usize, schema_end: usize, depth: usize)` | 就地执行 `jstvEmitEvent` 操作。 |
| `jstvWalkMapKeyword` | `(st: &mut JstvTraverseState, obj_start: usize, obj_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize)` | 就地执行 `jstvWalkMapKeyword` 操作。 |
| `jstvWalkArrayKeyword` | `(st: &mut JstvTraverseState, arr_start: usize, arr_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize)` | 就地执行 `jstvWalkArrayKeyword` 操作。 |
| `jstvWalkSchema` | `(st: &mut JstvTraverseState, schema_start: usize, schema_end: usize, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, depth: usize)` | 就地执行 `jstvWalkSchema` 操作。 |
| `traverseJsonSchema` | `(schema_json: string, out: []SchemaVisitEvent): usize` | 调用 `traverseJsonSchema` 并返回 `usize`。 |
| `countSchemaNodes` | `(schema_json: string): usize` | 调用 `countSchemaNodes` 并返回 `usize`。 |
| `findSubschemaByPointer` | `(schema_json: string, pointer: string): string?` | 查询或读取目标数据，返回 `string?`。 |

## 最小调用示例

```cp
import { jstvSkipWs } from "json-schema-traverse";

pub fn main() {
    let result = jstvSkipWs("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
