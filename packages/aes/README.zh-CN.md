[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# aes

Zero-allocation pure CP FIPS-197 AES-128, AES-192, and AES-256 block cipher (RustCrypto aes parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:aes` ([https://github.com/RustCrypto/block-ciphers](https://github.com/RustCrypto/block-ciphers))

| 元信息 | 值 |
| --- | --- |
| 包名 | `aes` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add aes
```

```cp
import {
    Aes128,
    Aes192,
    Aes256,
    aesXtime,
    aesMul,
    aesSubBytes,
    aesInvSubBytes,
    aesShiftRows,
} from "aes";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Aes128` | `class` | 字段: `rk: [176]u8` · 方法: `new, encryptBlock, decryptBlock` |
| `Aes192` | `class` | 字段: `rk: [208]u8` · 方法: `new, encryptBlock, decryptBlock` |
| `Aes256` | `class` | 字段: `rk: [240]u8` · 方法: `new, encryptBlock, decryptBlock` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `aesXtime` | `(x: u8): u8` | 调用 `aesXtime` 并返回 `u8`。 |
| `aesMul` | `(a_in: u8, b_in: u8): u8` | 调用 `aesMul` 并返回 `u8`。 |
| `aesSubBytes` | `(state: []u8)` | 就地执行 `aesSubBytes` 操作。 |
| `aesInvSubBytes` | `(state: []u8)` | 就地执行 `aesInvSubBytes` 操作。 |
| `aesShiftRows` | `(s: []u8)` | 就地执行 `aesShiftRows` 操作。 |
| `aesInvShiftRows` | `(s: []u8)` | 就地执行 `aesInvShiftRows` 操作。 |
| `aesMixColumns` | `(s: []u8)` | 就地执行 `aesMixColumns` 操作。 |
| `aesInvMixColumns` | `(s: []u8)` | 就地执行 `aesInvMixColumns` 操作。 |
| `aesAddRoundKey` | `(s: []u8, rk: &[]u8, round: usize)` | 就地执行 `aesAddRoundKey` 操作。 |
| `aesExpandKey` | `(key: &[]u8, nk: usize, nr: usize, rk: []u8)` | 就地执行 `aesExpandKey` 操作。 |
| `aesEncryptCore` | `(block: [16]u8, rk: &[]u8, nr: usize): [16]u8` | 调用 `aesEncryptCore` 并返回 `[16]u8`。 |
| `aesDecryptCore` | `(block: [16]u8, rk: &[]u8, nr: usize): [16]u8` | 调用 `aesDecryptCore` 并返回 `[16]u8`。 |

## 最小调用示例

```cp
import { aesXtime } from "aes";

pub fn main() {
    let result = aesXtime(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
