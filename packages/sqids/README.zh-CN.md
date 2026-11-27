[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sqids

Zero-allocation YouTube-style reversible short ID generator (Sqids specification) with deterministic shuffle, min-length padding, and profanity blocklist for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sqids` ([https://github.com/sqids/sqids-javascript](https://github.com/sqids/sqids-javascript))

| 元信息 | 值 |
| --- | --- |
| 包名 | `sqids` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sqids
```

```cp
import {
    SqidsError,
    SqidsOptions,
    Sqids,
    hasDigit,
    isBlockedId,
    shuffleAlphabet,
    reverseBytes,
    appendToId,
} from "sqids";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SqidsError` | `None, AlphabetTooShort, AlphabetTooLong, AlphabetNonAscii, AlphabetDuplicateChar` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SqidsOptions` | `class` | 字段: `alphabet: string, min_length: u8` |
| `Sqids` | `class` | 字段: `alphabet: [MAX_ALPHABET_LEN]u8, alphabet_len: usize, min_length: u8, valid: bool, err: SqidsError` · 方法: `initDefault, init, isValid, encode, encodeOne, decode, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hasDigit` | `(s: string): bool` | 执行 `hasDigit` 条件判定，返回 `bool`。 |
| `isBlockedId` | `(id: &[]u8): bool` | 执行 `isBlockedId` 条件判定，返回 `bool`。 |
| `shuffleAlphabet` | `(buf: []u8)` | 就地执行 `shuffleAlphabet` 操作。 |
| `reverseBytes` | `(buf: []u8)` | 就地执行 `reverseBytes` 操作。 |
| `appendToId` | `(num: u64, alphabet: &[]u8, dst: []u8, pos: usize): usize?` | 调用 `appendToId` 并返回 `usize?`。 |
| `chunkToNumber` | `(chunk: &[]u8, alphabet: &[]u8): u64?` | 调用 `chunkToNumber` 并返回 `u64?`。 |

## 最小调用示例

```cp
import { hasDigit } from "sqids";

pub fn main() {
    let result = hasDigit("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
