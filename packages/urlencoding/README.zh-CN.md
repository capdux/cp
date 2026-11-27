[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# urlencoding

Zero-heap RFC 3986 percent-encoding and decoding in pure CP (modeled after Rust urlencoding v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:urlencoding` ([https://github.com/kornelski/rust_urlencoding](https://github.com/kornelski/rust_urlencoding))（原库名 `urlencoding` → CP 包名 `urlencoding`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `urlencoding` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add urlencoding
```

```cp
import {
    urlencHexUpper,
    urlencHexVal,
    isUnreserved,
    encodedLen,
    encodeBinary,
    encode,
    decode,
    decodeLossy,
} from "urlencoding";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `urlencHexUpper` | `(nibble: u8): u8` | 调用 `urlencHexUpper` 并返回 `u8`。 |
| `urlencHexVal` | `(ch: u8): i16` | 调用 `urlencHexVal` 并返回 `i16`。 |
| `isUnreserved` | `(b: u8): bool` | 执行 `isUnreserved` 条件判定，返回 `bool`。 |
| `encodedLen` | `(input: string): usize` | 通过 `encodedLen` 编码或格式化输出，返回 `usize`。 |
| `encodeBinary` | `(data: &[]u8, out: []u8): usize` | 通过 `encodeBinary` 编码或格式化输出，返回 `usize`。 |
| `encode` | `(input: string, out: []u8): usize` | 通过 `encode` 编码或格式化输出，返回 `usize`。 |
| `decode` | `(input: string, out: []u8): i64` | 通过 `decode` 解析输入数据，返回 `i64`。 |
| `decodeLossy` | `(input: string, out: []u8): usize` | 通过 `decodeLossy` 解析输入数据，返回 `usize`。 |

## 最小调用示例

```cp
import { encodedLen } from "urlencoding";

pub fn main() {
    let result = encodedLen("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
