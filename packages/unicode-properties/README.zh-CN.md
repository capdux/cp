[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-properties

Zero-allocation Unicode character properties, UAX #31 identifier validation (XID_Start/XID_Continue), White_Space, Default_Ignorable, and UTS #51 Emoji classification in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-properties` ([https://github.com/unicode-rs/unicode-properties](https://github.com/unicode-rs/unicode-properties))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-properties` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-properties
```

```cp
import {
    decodeUtf8Strict,
    isXidStart,
    isXidContinue,
    isUnicodeWhitespace,
    isEmojiPresentation,
    isEmoji,
    isDefaultIgnorable,
    isValidIdentifier,
} from "unicode-properties";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decodeUtf8Strict` | `(s: string, pos: usize, next_pos: &mut usize): u32` | 通过 `decodeUtf8Strict` 解析输入数据，返回 `u32`。 |
| `isXidStart` | `(cp: u32): bool` | 执行 `isXidStart` 条件判定，返回 `bool`。 |
| `isXidContinue` | `(cp: u32): bool` | 执行 `isXidContinue` 条件判定，返回 `bool`。 |
| `isUnicodeWhitespace` | `(cp: u32): bool` | 执行 `isUnicodeWhitespace` 条件判定，返回 `bool`。 |
| `isEmojiPresentation` | `(cp: u32): bool` | 执行 `isEmojiPresentation` 条件判定，返回 `bool`。 |
| `isEmoji` | `(cp: u32): bool` | 执行 `isEmoji` 条件判定，返回 `bool`。 |
| `isDefaultIgnorable` | `(cp: u32): bool` | 执行 `isDefaultIgnorable` 条件判定，返回 `bool`。 |
| `isValidIdentifier` | `(s: string): bool` | 执行 `isValidIdentifier` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { decodeUtf8Strict } from "unicode-properties";

pub fn main() {
    let result = decodeUtf8Strict("example", 0, /* next_pos: &mut usize */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
