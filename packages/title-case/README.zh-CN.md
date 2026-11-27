[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# title-case

Zero-allocation AP/Chicago style English title-case, sentence-case, and capitalization engine in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:title-case` ([https://github.com/blakeembrey/change-case](https://github.com/blakeembrey/change-case)) / `crates.io:titlecase` ([https://github.com/wezm/titlecase](https://github.com/wezm/titlecase))

| 元信息 | 值 |
| --- | --- |
| 包名 | `title-case` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add title-case
```

```cp
import {
    isUpper,
    isLower,
    isSmallWord,
    isWordChar,
    isAllCapsAcronym,
    hasMoreWords,
    toTitleCase,
    toSentenceCase,
} from "title-case";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isUpper` | `(b: u8): bool` | 执行 `isUpper` 条件判定，返回 `bool`。 |
| `isLower` | `(b: u8): bool` | 执行 `isLower` 条件判定，返回 `bool`。 |
| `isSmallWord` | `(word: string): bool` | 执行 `isSmallWord` 条件判定，返回 `bool`。 |
| `isWordChar` | `(b: u8): bool` | 执行 `isWordChar` 条件判定，返回 `bool`。 |
| `isAllCapsAcronym` | `(word: string): bool` | 执行 `isAllCapsAcronym` 条件判定，返回 `bool`。 |
| `hasMoreWords` | `(input: string, start_pos: usize): bool` | 执行 `hasMoreWords` 条件判定，返回 `bool`。 |
| `toTitleCase` | `(input: string, out: []u8): usize` | 调用 `toTitleCase` 并返回 `usize`。 |
| `toSentenceCase` | `(input: string, out: []u8): usize` | 调用 `toSentenceCase` 并返回 `usize`。 |
| `capitalizeFirst` | `(input: string, out: []u8): usize` | 调用 `capitalizeFirst` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isSmallWord } from "title-case";

pub fn main() {
    let result = isSmallWord("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
