[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ed25519

Zero-heap RFC 8032 Ed25519 signature & key containers, canonical scalar S < L validation, RFC 8410 SPKI/PKCS#8 DER and hex codecs in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ed25519` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures)) — `Apache-2.0 OR MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `ed25519` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0 OR MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ed25519
```

```cp
import {
    Ed25519Signature,
    Ed25519PublicKey,
    Ed25519SecretKey,
    ed25Zero32,
    ed25Zero64,
    ed25GroupOrderL,
    ed25SpkiPrefix,
    ed25Pkcs8Prefix,
} from "ed25519";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Ed25519Signature` | `class` | 字段: `bytes: [64]u8` · 方法: `fromBytes, fromSlice, fromComponents, rBytes, sBytes, toBytes, +4 项` |
| `Ed25519PublicKey` | `class` | 字段: `bytes: [32]u8` · 方法: `fromBytes, fromSlice, toBytes, toSpkiDer, fromSpkiDer, fromHex, +2 项` |
| `Ed25519SecretKey` | `class` | 字段: `bytes: [32]u8` · 方法: `fromBytes, fromSlice, toBytes, toPkcs8Der, fromPkcs8Der, fromHex, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ed25Zero32` | `(): [32]u8` | 调用 `ed25Zero32` 并返回 `[32]u8`。 |
| `ed25Zero64` | `(): [64]u8` | 调用 `ed25Zero64` 并返回 `[64]u8`。 |
| `ed25GroupOrderL` | `(): [32]u8` | 调用 `ed25GroupOrderL` 并返回 `[32]u8`。 |
| `ed25SpkiPrefix` | `(): [12]u8` | 调用 `ed25SpkiPrefix` 并返回 `[12]u8`。 |
| `ed25Pkcs8Prefix` | `(): [16]u8` | 调用 `ed25Pkcs8Prefix` 并返回 `[16]u8`。 |
| `ed25HexNibble` | `(ch: u8): i32` | 调用 `ed25HexNibble` 并返回 `i32`。 |
| `ed25HexDigit` | `(n: u8): u8` | 调用 `ed25HexDigit` 并返回 `u8`。 |
| `ed25EncodeHex` | `(src: &[]u8, out: []u8): usize` | 调用 `ed25EncodeHex` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ed25Zero32 } from "ed25519";

pub fn main() {
    let result = ed25Zero32();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
