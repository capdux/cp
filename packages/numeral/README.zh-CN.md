[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# numeral

Zero-allocation human-readable number formatting (grouped thousands, EN/ZH compact suffixes, percentages, strict Roman numerals, and English words) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:numeral` ([https://github.com/adamwdraper/Numeral-js](https://github.com/adamwdraper/Numeral-js)) / `npm:filesize` ([https://github.com/avoidwork/filesize.js](https://github.com/avoidwork/filesize.js))（原库名 `numeral` → CP 包名 `numeral`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `numeral` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add numeral
```

```cp
import {
    appendByte,
    absU64,
    formatGrouped,
    writeCompactScaled,
    formatCompactEn,
    formatCompactZh,
    formatPercent,
    toRoman,
} from "numeral";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `absU64` | `(n: i64): u64` | 调用 `absU64` 并返回 `u64`。 |
| `formatGrouped` | `(n: i64, sep: u8, out: []u8): string` | 通过 `formatGrouped` 编码或格式化输出，返回 `string`。 |
| `writeCompactScaled` | `(abs: u64, unit: u64, suffix: string, next_threshold: u64, next_suffix: string, negative: bool, out: []u8): string` | 调用 `writeCompactScaled` 并返回 `string`。 |
| `formatCompactEn` | `(n: i64, out: []u8): string` | 通过 `formatCompactEn` 编码或格式化输出，返回 `string`。 |
| `formatCompactZh` | `(n: i64, out: []u8): string` | 通过 `formatCompactZh` 编码或格式化输出，返回 `string`。 |
| `formatPercent` | `(ratio: f64, decimals: u8, out: []u8): string` | 通过 `formatPercent` 编码或格式化输出，返回 `string`。 |
| `toRoman` | `(n: u16, out: []u8): string` | 调用 `toRoman` 并返回 `string`。 |
| `romanCharVal` | `(c: u8): u16` | 调用 `romanCharVal` 并返回 `u16`。 |
| `fromRoman` | `(s: string): u16?` | 调用 `fromRoman` 并返回 `u16?`。 |
| `onesWord` | `(n: u64): string` | 调用 `onesWord` 并返回 `string`。 |
| `tensWord` | `(n: u64): string` | 调用 `tensWord` 并返回 `string`。 |
| `appendUnder1000` | `(v: u64, out: []u8, start_pos: usize): usize` | 调用 `appendUnder1000` 并返回 `usize`。 |
| `appendScaleGroup` | `(chunk: u64, label: string, out: []u8, start_pos: usize, has_prev: bool): usize` | 调用 `appendScaleGroup` 并返回 `usize`。 |
| `toEnglishWords` | `(n: i64, out: []u8): string` | 调用 `toEnglishWords` 并返回 `string`。 |

## 最小调用示例

```cp
import { fromRoman } from "numeral";

pub fn main() {
    let result = fromRoman("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
