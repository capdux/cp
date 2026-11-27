[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# debug

Zero-allocation namespace debug log filter, glob exclusion engine, deterministic ANSI coloring, and diff-ms formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:debug` ([https://github.com/debug-js/debug](https://github.com/debug-js/debug))

| 元信息 | 值 |
| --- | --- |
| 包名 | `debug` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add debug
```

```cp
import {
    DebugFilter,
    DebugLogger,
    isSep,
    globMatch,
    appendByte,
    appendStr,
    appendU64,
    isNamespaceEnabled,
} from "debug";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DebugFilter` | `class` | 字段: `raw_pattern: string` · 方法: `from, isEnabled` |
| `DebugLogger` | `class` | 字段: `namespace: string, color_code: u8, last_ms: i64, enabled: bool` · 方法: `create, formatLine` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSep` | `(c: u8): bool` | 执行 `isSep` 条件判定，返回 `bool`。 |
| `globMatch` | `(pat: string, text: string): bool` | 调用 `globMatch` 并返回 `bool`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `appendU64` | `(out: []u8, pos: usize, val: u64): usize` | 调用 `appendU64` 并返回 `usize`。 |
| `isNamespaceEnabled` | `(pattern: string, namespace: string): bool` | 执行 `isNamespaceEnabled` 条件判定，返回 `bool`。 |
| `selectColor` | `(namespace: string): u8` | 调用 `selectColor` 并返回 `u8`。 |
| `formatDiffMs` | `(diff_ms: i64, out: []u8): string` | 通过 `formatDiffMs` 编码或格式化输出，返回 `string`。 |
| `formatDebugLine` | `(ns: string, msg: string, diff_ms: i64, use_color: bool, out: []u8): string` | 通过 `formatDebugLine` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { isNamespaceEnabled } from "debug";

pub fn main() {
    let result = isNamespaceEnabled("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
