[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pbkdf2

Pure CP zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256 password-based key derivation and constant-time verification

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`pbkdf2`](https://github.com/RustCrypto/password-hashes)（`MIT OR Apache-2.0`）（原库名 `pbkdf2`，CP 包名 `pbkdf2`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `pbkdf2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pbkdf2
```

```cp
import {
    Pbkdf2Pads,
    pbkdf2InitPads,
    pbkdf2HmacSaltBlock,
    pbkdf2Hmac32,
    pbkdf2HmacSha256,
    pbkdf2Verify,
    pbkdf2HmacSha256Str,
} from "pbkdf2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pbkdf2Pads` | `class` | 字段: `inner_base: Sha256State, outer_base: Sha256State` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pbkdf2InitPads` | `(password: &[]u8): Pbkdf2Pads` | 调用 `pbkdf2InitPads` 并返回 `Pbkdf2Pads`。 |
| `pbkdf2HmacSaltBlock` | `(pads: Pbkdf2Pads, salt: &[]u8, block_idx: u32, out32: []u8)` | 就地执行 `pbkdf2HmacSaltBlock` 操作。 |
| `pbkdf2Hmac32` | `(pads: Pbkdf2Pads, msg32: []u8, out32: []u8)` | 就地执行 `pbkdf2Hmac32` 操作。 |
| `pbkdf2HmacSha256` | `(password: &[]u8, salt: &[]u8, iterations: u32, out_dk: []u8): bool` | 调用 `pbkdf2HmacSha256` 并返回 `bool`。 |
| `pbkdf2Verify` | `(password: &[]u8, salt: &[]u8, iterations: u32, expected_dk: &[]u8): bool` | 调用 `pbkdf2Verify` 并返回 `bool`。 |
| `pbkdf2HmacSha256Str` | `(password: string, salt: string, iterations: u32, out_dk: []u8): bool` | 调用 `pbkdf2HmacSha256Str` 并返回 `bool`。 |

## 最小调用示例

```cp
import { pbkdf2HmacSha256Str } from "pbkdf2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pbkdf2HmacSha256Str("example", "example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
