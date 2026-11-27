[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# parse_duration

Zero-heap human-readable duration parser and formatter supporting compound expressions, fractional units, Unicode microseconds, and overflow protection (clean-room port of Rust parse_duration / duration-str)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:parse_duration` ([https://github.com/zeta12ti/parse_duration](https://github.com/zeta12ti/parse_duration))（原库名 `parse_duration` → CP 包名 `parse_duration`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `parse_duration` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add parse_duration
```

```cp
import {
    ParsedDuration,
    pdurCheckedAddU64,
    pdurCheckedMulU64,
    pdurMatchUnit,
    pdurAppendPart,
    parseDuration,
    formatDuration,
} from "parse_duration";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedDuration` | `class` | 字段: `secs: u64, nanos: u32` · 方法: `isZero, totalSecs, totalMillis, totalNanos` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pdurCheckedAddU64` | `(a: u64, b: u64): u64?` | 调用 `pdurCheckedAddU64` 并返回 `u64?`。 |
| `pdurCheckedMulU64` | `(a: u64, b: u64): u64?` | 调用 `pdurCheckedMulU64` 并返回 `u64?`。 |
| `pdurMatchUnit` | `(u: string): u32` | 调用 `pdurMatchUnit` 并返回 `u32`。 |
| `pdurAppendPart` | `(out: []u8, pos: usize, val: u64, suffix: string): usize` | 调用 `pdurAppendPart` 并返回 `usize`。 |
| `parseDuration` | `(input: string): ParsedDuration?` | 通过 `parseDuration` 解析输入数据，返回 `ParsedDuration?`。 |
| `formatDuration` | `(d: ParsedDuration, out: []u8): usize` | 通过 `formatDuration` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseDuration } from "parse_duration";

pub fn main() {
    let result = parseDuration("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
