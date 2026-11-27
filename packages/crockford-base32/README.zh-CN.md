[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# crockford-base32

Zero-allocation Douglas Crockford Base32 encoder/decoder for 64-bit integers (with optional mod-37 check symbol), arbitrary byte streams, and grouped formatting in CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:crockford-base32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `crockford-base32` → CP 包名 `crockford-base32`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `crockford-base32` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add crockford-base32
```

```cp
import {
    bcBuildDecode32Lut,
    bcBuildDecode37Lut,
    bcEncodeDigit32,
    bcEncodeCheck37,
    bcDecodeDigit32,
    bcDecodeCheck37,
    encodeU64,
    decodeU64,
} from "crockford-base32";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bcBuildDecode32Lut` | `(): [256]u8` | 调用 `bcBuildDecode32Lut` 并返回 `[256]u8`。 |
| `bcBuildDecode37Lut` | `(): [256]u8` | 调用 `bcBuildDecode37Lut` 并返回 `[256]u8`。 |
| `bcEncodeDigit32` | `(v: u8): u8` | 调用 `bcEncodeDigit32` 并返回 `u8`。 |
| `bcEncodeCheck37` | `(v: u8): u8` | 调用 `bcEncodeCheck37` 并返回 `u8`。 |
| `bcDecodeDigit32` | `(c: u8): u8` | 调用 `bcDecodeDigit32` 并返回 `u8`。 |
| `bcDecodeCheck37` | `(c: u8): u8` | 调用 `bcDecodeCheck37` 并返回 `u8`。 |
| `encodeU64` | `(val: u64, with_check: bool, out: []u8): usize` | 通过 `encodeU64` 编码或格式化输出，返回 `usize`。 |
| `decodeU64` | `(input: string, with_check: bool, out_val: &mut u64): bool` | 通过 `decodeU64` 解析输入数据，返回 `bool`。 |
| `encodeBytes` | `(data: &[]u8, out: []u8): usize` | 通过 `encodeBytes` 编码或格式化输出，返回 `usize`。 |
| `decodeBytes` | `(input: string, out: []u8): i64` | 通过 `decodeBytes` 解析输入数据，返回 `i64`。 |
| `formatGrouped` | `(raw: string, group_size: usize, out: []u8): usize` | 通过 `formatGrouped` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { decodeU64 } from "crockford-base32";

pub fn main() {
    let result = decodeU64("example", false, /* out_val: &mut u64 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
