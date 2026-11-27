[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bech32

Zero-allocation BIP-173 Bech32 and BIP-350 Bech32m encoder, decoder, 8-to-5 bit converter, and BCH checksum verifier for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bech32` ([https://github.com/rust-bitcoin/rust-bech32](https://github.com/rust-bitcoin/rust-bech32))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bech32` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bech32
```

```cp
import {
    Bech32Variant,
    Bech32Decoded,
    ParsedBech32Meta,
    charsetIndex,
    polymodStep,
    polymodHrp,
    variantConst,
    isValidHrp,
} from "bech32";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Bech32Variant` | `Bech32, Bech32m` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Bech32Decoded` | `class` | 字段: `valid: bool, variant: Bech32Variant, hrp: string, data: string` · 方法: `invalid` |
| `ParsedBech32Meta` | `class` | 字段: `valid: bool, variant: Bech32Variant, sep: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `charsetIndex` | `(c: u8): u8?` | 调用 `charsetIndex` 并返回 `u8?`。 |
| `polymodStep` | `(chk: u32, v: u8): u32` | 调用 `polymodStep` 并返回 `u32`。 |
| `polymodHrp` | `(hrp: string): u32` | 调用 `polymodHrp` 并返回 `u32`。 |
| `variantConst` | `(v: Bech32Variant): u32` | 调用 `variantConst` 并返回 `u32`。 |
| `isValidHrp` | `(hrp: string): bool` | 执行 `isValidHrp` 条件判定，返回 `bool`。 |
| `to5BitWords` | `(bytes: string, out: []u8): usize?` | 调用 `to5BitWords` 并返回 `usize?`。 |
| `from5BitWords` | `(words: &[]u8, out: []u8): usize?` | 调用 `from5BitWords` 并返回 `usize?`。 |
| `encodeBech32` | `(hrp: string, payload_bytes: string, variant: Bech32Variant, out: []u8): string?` | 通过 `encodeBech32` 编码或格式化输出，返回 `string?`。 |
| `validateStructureAndChecksum` | `(encoded: string): ParsedBech32Meta` | 校验输入合法性，返回 `ParsedBech32Meta`。 |
| `verifyBech32` | `(encoded: string): bool` | 校验输入合法性，返回 `bool`。 |
| `decodeBech32` | `(encoded: string, hrp_out: []u8, data_out: []u8): Bech32Decoded` | 通过 `decodeBech32` 解析输入数据，返回 `Bech32Decoded`。 |

## 最小调用示例

```cp
import { isValidHrp } from "bech32";

pub fn main() {
    let result = isValidHrp("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
