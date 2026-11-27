[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# emoji-regex

Zero-allocation Unicode RGI Emoji UTF-8 scanner and extractor (npm emoji-regex parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`emoji-regex`](https://github.com/mathiasbynens/emoji-regex)（`MIT`）（包名 `emoji-regex`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `emoji-regex` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add emoji-regex
```

```cp
import {
    EmojiSpan,
    EmjDecoded,
    emjDecodeUtf8,
    emjIsRegionalIndicator,
    emjIsSkinTone,
    emjIsKeycapBase,
    emjIsTagChar,
    isEmojiCodePoint,
} from "emoji-regex";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EmojiSpan` | `class` | 字段: `start: usize, end: usize` |
| `EmjDecoded` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emjDecodeUtf8` | `(s: string, pos: usize): EmjDecoded` | 调用 `emjDecodeUtf8` 并返回 `EmjDecoded`。 |
| `emjIsRegionalIndicator` | `(cp: u32): bool` | 调用 `emjIsRegionalIndicator` 并返回 `bool`。 |
| `emjIsSkinTone` | `(cp: u32): bool` | 调用 `emjIsSkinTone` 并返回 `bool`。 |
| `emjIsKeycapBase` | `(cp: u32): bool` | 调用 `emjIsKeycapBase` 并返回 `bool`。 |
| `emjIsTagChar` | `(cp: u32): bool` | 调用 `emjIsTagChar` 并返回 `bool`。 |
| `isEmojiCodePoint` | `(cp: u32): bool` | 执行 `isEmojiCodePoint` 条件判定，返回 `bool`。 |
| `emjConsumeModifiers` | `(s: string, start_pos: usize): usize` | 调用 `emjConsumeModifiers` 并返回 `usize`。 |
| `findNextEmoji` | `(s: string, from_pos: usize): EmojiSpan?` | 查询或读取目标数据，返回 `EmojiSpan?`。 |
| `countEmojis` | `(s: string): usize` | 调用 `countEmojis` 并返回 `usize`。 |
| `containsEmoji` | `(s: string): bool` | 调用 `containsEmoji` 并返回 `bool`。 |
| `stripEmojis` | `(s: string, out: []u8): usize` | 调用 `stripEmojis` 并返回 `usize`。 |

## 最小调用示例

```cp
import { emjDecodeUtf8 } from "emoji-regex";

pub fn main() {
    let result = emjDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
