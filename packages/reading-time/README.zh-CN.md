[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# reading-time

Zero-allocation bilingual EN/CJK document statistics, reading-time estimator, and LLM token estimator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:reading-time` ([https://github.com/ngryman/reading-time](https://github.com/ngryman/reading-time))（原库名 `reading-time` → CP 包名 `reading-time`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `reading-time` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add reading-time
```

```cp
import {
    TextStats,
    DecodedChar,
    decodeUtf8,
    isCjkChar,
    isWordCoreChar,
    isWsCp,
    isSentenceEnd,
    analyzeTextWithWpm,
} from "reading-time";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TextStats` | `class` | 字段: `bytes: usize, chars: usize, ascii_words: usize, cjk_chars: usize, total_words: usize, lines: usize, +6 项` · 方法: `empty` |
| `DecodedChar` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decodeUtf8` | `(s: string, pos: usize): DecodedChar` | 通过 `decodeUtf8` 解析输入数据，返回 `DecodedChar`。 |
| `isCjkChar` | `(cp: u32): bool` | 执行 `isCjkChar` 条件判定，返回 `bool`。 |
| `isWordCoreChar` | `(cp: u32): bool` | 执行 `isWordCoreChar` 条件判定，返回 `bool`。 |
| `isWsCp` | `(cp: u32): bool` | 执行 `isWsCp` 条件判定，返回 `bool`。 |
| `isSentenceEnd` | `(cp: u32): bool` | 执行 `isSentenceEnd` 条件判定，返回 `bool`。 |
| `analyzeTextWithWpm` | `(text: string, en_wpm: usize, cjk_cpm: usize): TextStats` | 调用 `analyzeTextWithWpm` 并返回 `TextStats`。 |
| `analyzeText` | `(text: string): TextStats` | 调用 `analyzeText` 并返回 `TextStats`。 |
| `formatReadingTime` | `(stats: TextStats, zh: bool, out: []u8): string` | 通过 `formatReadingTime` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { decodeUtf8 } from "reading-time";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
