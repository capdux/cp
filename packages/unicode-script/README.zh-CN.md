[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-script

Zero-heap UAX #24 Unicode Script property classifier and dominant script detector in pure CP (modeled after Rust unicode-script)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-script` ([https://github.com/unicode-rs/unicode-script](https://github.com/unicode-rs/unicode-script))（原库名 `unicode-script` → CP 包名 `unicode-script`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-script` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-script
```

```cp
import {
    Script,
    UscDecoded,
    uscScriptIndex,
    uscIndexToScript,
    scriptOf,
    scriptName,
    isLatin,
    isHan,
} from "unicode-script";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Script` | `Common, Latin, Greek, Cyrillic, Han, Hiragana, Katakana, Hangul ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UscDecoded` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `uscScriptIndex` | `(s: Script): usize` | 调用 `uscScriptIndex` 并返回 `usize`。 |
| `uscIndexToScript` | `(idx: usize): Script` | 调用 `uscIndexToScript` 并返回 `Script`。 |
| `scriptOf` | `(cp: u32): Script` | 调用 `scriptOf` 并返回 `Script`。 |
| `scriptName` | `(s: Script): string` | 调用 `scriptName` 并返回 `string`。 |
| `isLatin` | `(cp: u32): bool` | 执行 `isLatin` 条件判定，返回 `bool`。 |
| `isHan` | `(cp: u32): bool` | 执行 `isHan` 条件判定，返回 `bool`。 |
| `isCjkScript` | `(s: Script): bool` | 执行 `isCjkScript` 条件判定，返回 `bool`。 |
| `uscDecodeUtf8` | `(text: string, pos: usize): UscDecoded` | 调用 `uscDecodeUtf8` 并返回 `UscDecoded`。 |
| `dominantScript` | `(text: string): Script` | 调用 `dominantScript` 并返回 `Script`。 |

## 最小调用示例

```cp
import { uscDecodeUtf8 } from "unicode-script";

pub fn main() {
    let result = uscDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
