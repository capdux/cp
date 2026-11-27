[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-bidi

Zero-allocation Unicode Bidirectional Algorithm (UAX #9) classifier, base direction resolver, visual run splitter, and bracket mirroring for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-bidi` ([https://github.com/servo/unicode-bidi](https://github.com/servo/unicode-bidi)) / `npm:bidi-js` ([https://github.com/lojjic/bidi-js](https://github.com/lojjic/bidi-js))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-bidi` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-bidi
```

```cp
import {
    BidiRun,
    Utf8Step,
    decodeUtf8At,
    bidiClass,
    baseDirection,
    hasRtl,
    isLtrOnly,
    resolveCharLevel,
} from "unicode-bidi";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BidiRun` | `class` | 字段: `start: usize, len: usize, level: u8` |
| `Utf8Step` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decodeUtf8At` | `(text: string, pos: usize): Utf8Step` | 通过 `decodeUtf8At` 解析输入数据，返回 `Utf8Step`。 |
| `bidiClass` | `(cp: u32): u8` | 调用 `bidiClass` 并返回 `u8`。 |
| `baseDirection` | `(text: string): u8` | 调用 `baseDirection` 并返回 `u8`。 |
| `hasRtl` | `(text: string): bool` | 执行 `hasRtl` 条件判定，返回 `bool`。 |
| `isLtrOnly` | `(text: string): bool` | 执行 `isLtrOnly` 条件判定，返回 `bool`。 |
| `resolveCharLevel` | `(cls: u8, base_level: u8, prev_level: u8): u8` | 调用 `resolveCharLevel` 并返回 `u8`。 |
| `computeVisualRuns` | `(text: string, out_runs: []BidiRun): usize` | 调用 `computeVisualRuns` 并返回 `usize`。 |
| `mirrorAsciiBracket` | `(ch: u8): u8` | 调用 `mirrorAsciiBracket` 并返回 `u8`。 |

## 最小调用示例

```cp
import { decodeUtf8At } from "unicode-bidi";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
