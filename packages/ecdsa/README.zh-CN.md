[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ecdsa

Pure CP zero-allocation ECDSA signature codec (RFC 3279 ASN.1 DER <-> fixed r||s for P-256/P-384, BIP-62 Low-S normalization, constant-time equality via der-asn1 and subtle-ct)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ecdsa` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures))（原库名 `ecdsa` → CP 包名 `ecdsa`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ecdsa` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ecdsa
```

```cp
import {
    EcdsaSigResult,
    EcdsaSignature,
    ecsigZero48,
    ecsigIsAllZero,
    ecsigP256Order,
    ecsigP256HalfOrder,
    ecsigCmpBe32,
    ecsigWriteBeInt,
} from "ecdsa";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EcdsaSigResult` | `class` | 字段: `ok: bool, sig: EcdsaSignature, err: i64` |
| `EcdsaSignature` | `class` | 字段: `r: [48]u8, s: [48]u8, coord_len: usize` · 方法: `empty, fromFixed, fromDer, toFixed, toDer, isLowSP256, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ecsigZero48` | `(): [48]u8` | 调用 `ecsigZero48` 并返回 `[48]u8`。 |
| `ecsigIsAllZero` | `(data: &[]u8): bool` | 调用 `ecsigIsAllZero` 并返回 `bool`。 |
| `ecsigP256Order` | `(): [32]u8` | 调用 `ecsigP256Order` 并返回 `[32]u8`。 |
| `ecsigP256HalfOrder` | `(): [32]u8` | 调用 `ecsigP256HalfOrder` 并返回 `[32]u8`。 |
| `ecsigCmpBe32` | `(a: &[]u8, b: &[]u8): i32` | 调用 `ecsigCmpBe32` 并返回 `i32`。 |
| `ecsigWriteBeInt` | `(w: &mut DerWriter, val: &[]u8): bool` | 调用 `ecsigWriteBeInt` 并返回 `bool`。 |
| `ecsigReadBeInt` | `(r: &mut DerReader, coord_len: usize, out: []u8): i64` | 调用 `ecsigReadBeInt` 并返回 `i64`。 |

## 最小调用示例

```cp
import { ecsigZero48 } from "ecdsa";

pub fn main() {
    let result = ecsigZero48();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
