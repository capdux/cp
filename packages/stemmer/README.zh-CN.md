[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# stemmer

Zero-allocation Martin Porter (1980) English Stemmer (Steps 1a-5b) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:stemmer` ([https://github.com/words/stemmer](https://github.com/words/stemmer)) / `crates.io:rust-stemmers` ([https://github.com/CurrySoftware/rust-stemmers](https://github.com/CurrySoftware/rust-stemmers)) / `crates.io:porter-stemmer` ([https://github.com/jwilm/porter-stemmer](https://github.com/jwilm/porter-stemmer))（原库名 `stemmer` → CP 包名 `stemmer`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `stemmer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add stemmer
```

```cp
import {
    isConsonant,
    measurePrefix,
    measure,
    containsVowel,
    endsDoubleConsonant,
    endsCvc,
    tryStep2,
    tryStep3,
} from "stemmer";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isConsonant` | `(buf: &[]u8, idx: usize): bool` | 执行 `isConsonant` 条件判定，返回 `bool`。 |
| `measurePrefix` | `(buf: &[]u8, end: usize): usize` | 调用 `measurePrefix` 并返回 `usize`。 |
| `measure` | `(word: &[]u8): usize` | 调用 `measure` 并返回 `usize`。 |
| `containsVowel` | `(buf: &[]u8, end: usize): bool` | 调用 `containsVowel` 并返回 `bool`。 |
| `endsDoubleConsonant` | `(buf: &[]u8, end: usize): bool` | 调用 `endsDoubleConsonant` 并返回 `bool`。 |
| `endsCvc` | `(buf: &[]u8, end: usize): bool` | 调用 `endsCvc` 并返回 `bool`。 |
| `tryStep2` | `(buf: []u8, len: usize): usize` | 调用 `tryStep2` 并返回 `usize`。 |
| `tryStep3` | `(buf: []u8, len: usize): usize` | 调用 `tryStep3` 并返回 `usize`。 |
| `tryStep4` | `(buf: []u8, len: usize): usize` | 调用 `tryStep4` 并返回 `usize`。 |
| `stem` | `(word: string, out: []u8): usize` | 调用 `stem` 并返回 `usize`。 |
| `sameStem` | `(a: string, b: string): bool` | 调用 `sameStem` 并返回 `bool`。 |

## 最小调用示例

```cp
import { stem } from "stemmer";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = stem("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
