[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# words-count

Zero-allocation multilingual (Latin + CJK) word/character/line counter and reading-time estimator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:words-count` ([https://github.com/magiclen/words-count](https://github.com/magiclen/words-count)) / `npm:word-count` ([https://github.com/yuehu/word-count](https://github.com/yuehu/word-count))（原库名 `words-count` → CP 包名 `words-count`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `words-count` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add words-count
```

```cp
import {
    TextCounts,
    isCjkCodepoint,
    wcIsLatinWordChar,
    wcNextCodepoint,
    analyze,
    countWords,
    estimateReadingSeconds,
} from "words-count";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TextCounts` | `class` | 字段: `bytes: usize, chars: usize, words: usize, cjk_chars: usize, latin_words: usize, lines: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isCjkCodepoint` | `(cp: u32): bool` | 执行 `isCjkCodepoint` 条件判定，返回 `bool`。 |
| `wcIsLatinWordChar` | `(cp: u32): bool` | 调用 `wcIsLatinWordChar` 并返回 `bool`。 |
| `wcNextCodepoint` | `(text: &[]u8, pos: usize, next_pos: &mut usize): u32` | 调用 `wcNextCodepoint` 并返回 `u32`。 |
| `analyze` | `(text: string): TextCounts` | 调用 `analyze` 并返回 `TextCounts`。 |
| `countWords` | `(text: string): usize` | 调用 `countWords` 并返回 `usize`。 |
| `estimateReadingSeconds` | `(text: string, words_per_minute: u32): u32` | 调用 `estimateReadingSeconds` 并返回 `u32`。 |

## 最小调用示例

```cp
import { analyze } from "words-count";

pub fn main() {
    let result = analyze("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
