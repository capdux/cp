[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cbc

Zero-allocation pure CP AES-128-CBC and AES-256-CBC block cipher mode with PKCS#7 padding (RustCrypto cbc parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cbc` ([https://github.com/RustCrypto/block-modes](https://github.com/RustCrypto/block-modes))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cbc` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cbc
```

```cp
import {
    cbcmReadBlock,
    cbcmWriteBlock,
    cbcmXorBlock,
    cbcmValidatePkcs7,
    aes128CbcEncryptBlocks,
    aes128CbcDecryptBlocks,
    aes128CbcEncryptPkcs7,
    aes128CbcDecryptPkcs7,
} from "cbc";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cbcmReadBlock` | `(src: &[]u8, offset: usize): [16]u8` | 调用 `cbcmReadBlock` 并返回 `[16]u8`。 |
| `cbcmWriteBlock` | `(dst: []u8, offset: usize, blk: [16]u8)` | 就地执行 `cbcmWriteBlock` 操作。 |
| `cbcmXorBlock` | `(a: [16]u8, b: [16]u8): [16]u8` | 调用 `cbcmXorBlock` 并返回 `[16]u8`。 |
| `cbcmValidatePkcs7` | `(lastBlock: [16]u8): i64` | 调用 `cbcmValidatePkcs7` 并返回 `i64`。 |
| `aes128CbcEncryptBlocks` | `(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | 调用 `aes128CbcEncryptBlocks` 并返回 `usize`。 |
| `aes128CbcDecryptBlocks` | `(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize` | 调用 `aes128CbcDecryptBlocks` 并返回 `usize`。 |
| `aes128CbcEncryptPkcs7` | `(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | 调用 `aes128CbcEncryptPkcs7` 并返回 `usize`。 |
| `aes128CbcDecryptPkcs7` | `(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64` | 调用 `aes128CbcDecryptPkcs7` 并返回 `i64`。 |
| `aes256CbcEncryptBlocks` | `(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | 调用 `aes256CbcEncryptBlocks` 并返回 `usize`。 |
| `aes256CbcDecryptBlocks` | `(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize` | 调用 `aes256CbcDecryptBlocks` 并返回 `usize`。 |
| `aes256CbcEncryptPkcs7` | `(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | 调用 `aes256CbcEncryptPkcs7` 并返回 `usize`。 |
| `aes256CbcDecryptPkcs7` | `(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64` | 调用 `aes256CbcDecryptPkcs7` 并返回 `i64`。 |

## 最小调用示例

```cp
import { cbcmReadBlock } from "cbc";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cbcmReadBlock(out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
