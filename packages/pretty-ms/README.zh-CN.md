[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pretty-ms

Zero-allocation millisecond decomposer and human-readable duration formatter composing parse-ms for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-ms` ([https://github.com/sindresorhus/pretty-ms](https://github.com/sindresorhus/pretty-ms))

| 元信息 | 值 |
| --- | --- |
| 包名 | `pretty-ms` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pretty-ms
```

```cp
import {
    PrettyMsOptions,
    prettyAbsI64,
    pushByte,
    pushStr,
    pushU64,
    pushTwoDigits,
    emitUnit,
    prettyMs,
} from "pretty-ms";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PrettyMsOptions` | `class` | 字段: `verbose: bool, compact: bool, colon_notation: bool, separate_ms: bool, unit_count: u8` · 方法: `defaults` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `prettyAbsI64` | `(v: i64): u64` | 调用 `prettyAbsI64` 并返回 `u64`。 |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | 写入或更新状态数据。 |
| `pushU64` | `(out: []u8, pos: usize, val: u64): usize` | 写入或更新状态数据。 |
| `pushTwoDigits` | `(out: []u8, pos: usize, val: u8): usize` | 写入或更新状态数据。 |
| `emitUnit` | `(out: []u8, pos: usize, emitted: u8, val: u64, short_unit: string, singular: string, plural: string, verbose: bool): usize` | 调用 `emitUnit` 并返回 `usize`。 |
| `prettyMs` | `(ms: i64, out: []u8): string` | 调用 `prettyMs` 并返回 `string`。 |
| `prettyMsWith` | `(ms: i64, opts: PrettyMsOptions, out: []u8): string` | 调用 `prettyMsWith` 并返回 `string`。 |

## 最小调用示例

```cp
import { pushStr } from "pretty-ms";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pushStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
