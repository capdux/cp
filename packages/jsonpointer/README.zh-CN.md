[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jsonpointer

Zero-allocation RFC 6901 JSON Pointer validator, escaper, segment navigator, array index parser, and iterator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsonpointer` ([https://github.com/janl/node-jsonpointer](https://github.com/janl/node-jsonpointer))（原库名 `jsonpointer` → CP 包名 `jsonpointer`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `jsonpointer` |
| 版本 | `0.1.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jsonpointer
```

```cp
import {
    PointerIter,
    isValid,
    escapeToken,
    unescapeToken,
    countSegments,
    hasSegment,
    getSegment,
    parseArrayIndex,
} from "jsonpointer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PointerIter` | `class` | 字段: `ptr: string, pos: usize, valid: bool` · 方法: `init, next` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isValid` | `(ptr: string): bool` | 执行 `isValid` 条件判定，返回 `bool`。 |
| `escapeToken` | `(token: string, out: []u8): usize` | 调用 `escapeToken` 并返回 `usize`。 |
| `unescapeToken` | `(token: string, out: []u8): usize` | 调用 `unescapeToken` 并返回 `usize`。 |
| `countSegments` | `(ptr: string): usize` | 调用 `countSegments` 并返回 `usize`。 |
| `hasSegment` | `(ptr: string, index: usize): bool` | 执行 `hasSegment` 条件判定，返回 `bool`。 |
| `getSegment` | `(ptr: string, index: usize, out: []u8): usize` | 查询或读取目标数据，返回 `usize`。 |
| `parseArrayIndex` | `(token: string): i64` | 通过 `parseArrayIndex` 解析输入数据，返回 `i64`。 |
| `appendSegment` | `(base: string, token: string, out: []u8): usize` | 调用 `appendSegment` 并返回 `usize`。 |
| `parentPointer` | `(ptr: string): string` | 调用 `parentPointer` 并返回 `string`。 |

## 最小调用示例

```cp
import { isValid } from "jsonpointer";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
