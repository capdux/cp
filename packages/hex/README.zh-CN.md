[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hex

Zero-allocation hexadecimal encoder/decoder, fixed-width integer hex formatter, and canonical HexDump utility for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hex` ([https://github.com/KokaKiwi/rust-hex](https://github.com/KokaKiwi/rust-hex))

| 元信息 | 值 |
| --- | --- |
| 包名 | `hex` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hex
```

```cp
import {
    appendByte,
    encodeHex,
    encodeHexUpper,
    isValidHex,
    decodeHex,
    decodeHexPrefixed,
    encodeU32Hex,
    encodeU64Hex,
} from "hex";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `encodeHex` | `(input: string, out: []u8): string` | 通过 `encodeHex` 编码或格式化输出，返回 `string`。 |
| `encodeHexUpper` | `(input: string, out: []u8): string` | 通过 `encodeHexUpper` 编码或格式化输出，返回 `string`。 |
| `isValidHex` | `(s: string): bool` | 执行 `isValidHex` 条件判定，返回 `bool`。 |
| `decodeHex` | `(hex_str: string, out: []u8): string?` | 通过 `decodeHex` 解析输入数据，返回 `string?`。 |
| `decodeHexPrefixed` | `(hex_str: string, out: []u8): string?` | 通过 `decodeHexPrefixed` 解析输入数据，返回 `string?`。 |
| `encodeU32Hex` | `(v: u32, out: []u8): string` | 通过 `encodeU32Hex` 编码或格式化输出，返回 `string`。 |
| `encodeU64Hex` | `(v: u64, out: []u8): string` | 通过 `encodeU64Hex` 编码或格式化输出，返回 `string`。 |
| `hexDump` | `(data: string, out: []u8): string` | 调用 `hexDump` 并返回 `string`。 |

## 最小调用示例

```cp
import { encodeHex } from "hex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeHex("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
