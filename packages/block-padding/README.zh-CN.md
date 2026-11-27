[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# block-padding

Zero-allocation PKCS#7 (constant-time padding-oracle-resistant unpad), ISO 7816-4, ANSI X9.23, ZeroPadding, 128-bit BE/LE CTR counter increment, and XOR block primitives reusing subtle-ct in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:block-padding` ([https://github.com/RustCrypto/utils](https://github.com/RustCrypto/utils))

| 元信息 | 值 |
| --- | --- |
| 包名 | `block-padding` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add block-padding
```

```cp
import {
    bpCopyBytes,
    pkcs7Pad,
    pkcs7UnpadCt,
    iso7816Pad,
    iso7816Unpad,
    ansiX923Pad,
    ansiX923Unpad,
    zeroPad,
} from "block-padding";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bpCopyBytes` | `(dst: []u8, src: &[]u8)` | 就地执行 `bpCopyBytes` 操作。 |
| `pkcs7Pad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | 调用 `pkcs7Pad` 并返回 `i64`。 |
| `pkcs7UnpadCt` | `(padded: &[]u8, block_size: usize): i64` | 调用 `pkcs7UnpadCt` 并返回 `i64`。 |
| `iso7816Pad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | 执行 `iso7816Pad` 条件判定，返回 `bool`。 |
| `iso7816Unpad` | `(padded: &[]u8, block_size: usize): i64` | 执行 `iso7816Unpad` 条件判定，返回 `bool`。 |
| `ansiX923Pad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | 调用 `ansiX923Pad` 并返回 `i64`。 |
| `ansiX923Unpad` | `(padded: &[]u8, block_size: usize): i64` | 调用 `ansiX923Unpad` 并返回 `i64`。 |
| `zeroPad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | 调用 `zeroPad` 并返回 `i64`。 |
| `zeroUnpad` | `(padded: &[]u8, block_size: usize): i64` | 调用 `zeroUnpad` 并返回 `i64`。 |
| `ctrIncBe128` | `(counter: []u8)` | 就地执行 `ctrIncBe128` 操作。 |
| `ctrIncLe128` | `(counter: []u8)` | 就地执行 `ctrIncLe128` 操作。 |
| `xorBlock` | `(dst: []u8, a: &[]u8, b: &[]u8): bool` | 调用 `xorBlock` 并返回 `bool`。 |
| `wipeBlock` | `(buf: []u8)` | 就地执行 `wipeBlock` 操作。 |

## 最小调用示例

```cp
import { iso7816Pad } from "block-padding";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = iso7816Pad(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
