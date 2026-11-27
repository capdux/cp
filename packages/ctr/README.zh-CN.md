[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ctr

Zero-allocation NIST SP 800-38A AES-CTR stream cipher modes (Ctr128BE / Ctr128LE) with O(1) seek in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ctr` ([https://github.com/RustCrypto/block-modes](https://github.com/RustCrypto/block-modes)) — `MIT OR Apache-2.0`

| 元信息 | 值 |
| --- | --- |
| 包名 | `ctr` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ctr
```

```cp
import {
    Aes128Ctr128Be,
    Aes128Ctr128Le,
    Aes256Ctr128Be,
    Aes256Ctr128Le,
    ctrZeroBlock,
    ctrAddBe128,
    ctrAddLe128,
} from "ctr";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Aes128Ctr128Be` | `class` | 字段: `cipher: Aes128, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · 方法: `new, currentPos, seek, applyKeystream` |
| `Aes128Ctr128Le` | `class` | 字段: `cipher: Aes128, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · 方法: `new, currentPos, seek, applyKeystream` |
| `Aes256Ctr128Be` | `class` | 字段: `cipher: Aes256, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · 方法: `new, currentPos, seek, applyKeystream` |
| `Aes256Ctr128Le` | `class` | 字段: `cipher: Aes256, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · 方法: `new, currentPos, seek, applyKeystream` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ctrZeroBlock` | `(): [16]u8` | 调用 `ctrZeroBlock` 并返回 `[16]u8`。 |
| `ctrAddBe128` | `(iv: [16]u8, delta: u64): [16]u8` | 调用 `ctrAddBe128` 并返回 `[16]u8`。 |
| `ctrAddLe128` | `(iv: [16]u8, delta: u64): [16]u8` | 调用 `ctrAddLe128` 并返回 `[16]u8`。 |

## 最小调用示例

```cp
import { ctrZeroBlock } from "ctr";

pub fn main() {
    let result = ctrZeroBlock();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
