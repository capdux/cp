[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rfc6979

Pure CP zero-allocation RFC 6979 deterministic ECDSA nonce generator (HMAC-SHA256 DRBG, RustCrypto rfc6979 parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rfc6979` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures))

| 元信息 | 值 |
| --- | --- |
| 包名 | `rfc6979` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rfc6979
```

```cp
import {
    HmacDrbg256,
    rfcCopy32,
    rfcCmpBe32,
    rfcIsNonZero32,
    rfcOrderValid32,
    rfcSubBe32,
    generateK,
} from "rfc6979";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HmacDrbg256` | `class` | 字段: `k: [32]u8, v: [32]u8` · 方法: `init, nextBytes` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rfcCopy32` | `(dst: []u8, src: []u8)` | 就地执行 `rfcCopy32` 操作。 |
| `rfcCmpBe32` | `(a: []u8, b: []u8): i32` | 调用 `rfcCmpBe32` 并返回 `i32`。 |
| `rfcIsNonZero32` | `(a: []u8): bool` | 调用 `rfcIsNonZero32` 并返回 `bool`。 |
| `rfcOrderValid32` | `(q: []u8): bool` | 调用 `rfcOrderValid32` 并返回 `bool`。 |
| `rfcSubBe32` | `(a: []u8, b: []u8, out: []u8)` | 就地执行 `rfcSubBe32` 操作。 |
| `generateK` | `(priv_key_32: []u8, msg_hash_32: []u8, order_32: []u8, out_k32: []u8): bool` | 调用 `generateK` 并返回 `bool`。 |

## 最小调用示例

```cp
import { rfcCmpBe32 } from "rfc6979";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = rfcCmpBe32(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
