[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fill-range

Zero-heap numeric, alphabetic, stepped, and zero-padded range expansion engine (npm fill-range parity).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fill-range` ([https://github.com/jonschlinkert/fill-range](https://github.com/jonschlinkert/fill-range))

| 元信息 | 值 |
| --- | --- |
| 包名 | `fill-range` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fill-range
```

```cp
import {
    ParsedInt,
    isAsciiAlpha,
    absStepU64,
    absDiffI64,
    countRange,
    expandNumbers,
    expandChars,
    parseRangeInt,
} from "fill-range";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedInt` | `class` | 字段: `value: i64, pad_width: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isAsciiAlpha` | `(c: u8): bool` | 执行 `isAsciiAlpha` 条件判定，返回 `bool`。 |
| `absStepU64` | `(step: i64): u64` | 调用 `absStepU64` 并返回 `u64`。 |
| `absDiffI64` | `(a: i64, b: i64): u64` | 调用 `absDiffI64` 并返回 `u64`。 |
| `countRange` | `(start: i64, end: i64, step: i64): usize` | 调用 `countRange` 并返回 `usize`。 |
| `expandNumbers` | `(start: i64, end: i64, step: i64, out: []i64): i64` | 调用 `expandNumbers` 并返回 `i64`。 |
| `expandChars` | `(start: u8, end: u8, step: usize, out: []u8): i64` | 调用 `expandChars` 并返回 `i64`。 |
| `parseRangeInt` | `(s: &[]u8): ParsedInt` | 通过 `parseRangeInt` 解析输入数据，返回 `ParsedInt`。 |
| `writeSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | 调用 `writeSlice` 并返回 `i64`。 |
| `writePaddedInt` | `(dst: []u8, start_pos: usize, val: i64, pad_width: usize): i64` | 调用 `writePaddedInt` 并返回 `i64`。 |
| `formatRange` | `(start: &[]u8, end: &[]u8, step: i64, sep: &[]u8, dst: []u8): i64` | 通过 `formatRange` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { isAsciiAlpha } from "fill-range";

pub fn main() {
    let result = isAsciiAlpha(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
