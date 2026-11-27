[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hi-base32

Zero-allocation RFC 4648 Section 6/7 standard Base32 (A-Z2-7) and Base32hex (0-9A-V) encoder and decoder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hi-base32` ([https://github.com/emn178/hi-base32](https://github.com/emn178/hi-base32))（原库名 `hi-base32` → CP 包名 `hi-base32`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `hi-base32` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hi-base32
```

```cp
import {
    decodeStdChar,
    decodeHexChar,
    encodedLen,
    encodeWithAlphabet,
    decodeWithMode,
    encode,
    decode,
    encodeHex,
} from "hi-base32";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decodeStdChar` | `(b: u8): i32` | 通过 `decodeStdChar` 解析输入数据，返回 `i32`。 |
| `decodeHexChar` | `(b: u8): i32` | 通过 `decodeHexChar` 解析输入数据，返回 `i32`。 |
| `encodedLen` | `(byte_len: usize, pad: bool): usize` | 通过 `encodedLen` 编码或格式化输出，返回 `usize`。 |
| `encodeWithAlphabet` | `(data: &[]u8, pad: bool, alphabet: string, out: []u8): usize` | 通过 `encodeWithAlphabet` 编码或格式化输出，返回 `usize`。 |
| `decodeWithMode` | `(input: string, is_hex: bool, out: []u8): i64` | 通过 `decodeWithMode` 解析输入数据，返回 `i64`。 |
| `encode` | `(data: &[]u8, pad: bool, out: []u8): usize` | 通过 `encode` 编码或格式化输出，返回 `usize`。 |
| `decode` | `(input: string, out: []u8): i64` | 通过 `decode` 解析输入数据，返回 `i64`。 |
| `encodeHex` | `(data: &[]u8, pad: bool, out: []u8): usize` | 通过 `encodeHex` 编码或格式化输出，返回 `usize`。 |
| `decodeHex` | `(input: string, out: []u8): i64` | 通过 `decodeHex` 解析输入数据，返回 `i64`。 |

## 最小调用示例

```cp
import { decodeWithMode } from "hi-base32";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeWithMode("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
