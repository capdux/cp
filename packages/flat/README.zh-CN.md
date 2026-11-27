[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# flat

Zero-allocation JSON flattening (a.b.0.c), dot-prop path lookup & flat-to-JSON reconstruction engine (flat / dot-prop parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:flat` ([https://github.com/hughsk/flat](https://github.com/hughsk/flat))（原库名 `flat` → CP 包名 `flat`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `flat` |
| 版本 | `1.0.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add flat
```

```cp
import {
    FlatResult,
    strEq,
    isWs,
    skipWs,
    parseJsonStringEnd,
    formatUsize,
    parseValue,
    flattenJsonWith,
} from "flat";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FlatResult` | `class` | 字段: `pool: [MAX_FLAT_POOL]u8, pool_len: usize, key_starts: [MAX_FLAT_ENTRIES]usize, key_lens: [MAX_FLAT_ENTRIES]usize, val_starts: [MAX_FLAT_ENTRIES]usize, val_lens: [MAX_FLAT_ENTRIES]usize, +2 项` · 方法: `empty, pushEntry, keyAt, valAt, get, has` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `strEq` | `(a: string, b: string): bool` | 调用 `strEq` 并返回 `bool`。 |
| `isWs` | `(c: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `skipWs` | `(s: string, pos: usize): usize` | 调用 `skipWs` 并返回 `usize`。 |
| `parseJsonStringEnd` | `(s: string, open_quote: usize): usize` | 通过 `parseJsonStringEnd` 解析输入数据，返回 `usize`。 |
| `formatUsize` | `(v: usize, buf: []u8): string` | 通过 `formatUsize` 编码或格式化输出，返回 `string`。 |
| `parseValue` | `(s: string, start_pos: usize, prefix: []u8, prefix_len: usize, sep: u8, depth: usize, res: &mut FlatResult): usize` | 通过 `parseValue` 解析输入数据，返回 `usize`。 |
| `flattenJsonWith` | `(json_str: string, sep: u8): FlatResult` | 调用 `flattenJsonWith` 并返回 `FlatResult`。 |
| `flattenJson` | `(json_str: string): FlatResult` | 调用 `flattenJson` 并返回 `FlatResult`。 |
| `getByPath` | `(json_str: string, dot_path: string, out: []u8): string?` | 查询或读取目标数据，返回 `string?`。 |
| `hasPath` | `(json_str: string, dot_path: string): bool` | 执行 `hasPath` 条件判定，返回 `bool`。 |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `formatFlatJsonObject` | `(flat: FlatResult, out: []u8): string` | 通过 `formatFlatJsonObject` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { parseJsonStringEnd } from "flat";

pub fn main() {
    let result = parseJsonStringEnd("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
