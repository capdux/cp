[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# chacha20

Zero-allocation RFC 8439 ChaCha20 stream cipher (Rust chacha20 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:chacha20` ([https://github.com/RustCrypto/stream-ciphers](https://github.com/RustCrypto/stream-ciphers))（原库名 `chacha20` → CP 包名 `chacha20`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `chacha20` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add chacha20
```

```cp
import {
    ChaCha20Cipher,
    cc20Add32,
    cc20Rotl16,
    cc20Rotl12,
    cc20Rotl8,
    cc20Rotl7,
    cc20ReadLe32,
    cc20WriteLe32,
} from "chacha20";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ChaCha20Cipher` | `class` | 字段: `key: [32]u8, nonce: [12]u8, counter: u32, block_buf: [64]u8, block_pos: usize` · 方法: `new, newWithCounter, seek, applyKeystream` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cc20Add32` | `(a: u32, b: u32): u32` | 调用 `cc20Add32` 并返回 `u32`。 |
| `cc20Rotl16` | `(x: u32): u32` | 调用 `cc20Rotl16` 并返回 `u32`。 |
| `cc20Rotl12` | `(x: u32): u32` | 调用 `cc20Rotl12` 并返回 `u32`。 |
| `cc20Rotl8` | `(x: u32): u32` | 调用 `cc20Rotl8` 并返回 `u32`。 |
| `cc20Rotl7` | `(x: u32): u32` | 调用 `cc20Rotl7` 并返回 `u32`。 |
| `cc20ReadLe32` | `(b: []u8, off: usize): u32` | 调用 `cc20ReadLe32` 并返回 `u32`。 |
| `cc20WriteLe32` | `(out: []u8, off: usize, v: u32)` | 就地执行 `cc20WriteLe32` 操作。 |
| `cc20QuarterRound` | `(st: []u32, ai: usize, bi: usize, ci: usize, di: usize)` | 就地执行 `cc20QuarterRound` 操作。 |
| `chacha20Block` | `(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool` | 调用 `chacha20Block` 并返回 `bool`。 |

## 最小调用示例

```cp
import { cc20Add32 } from "chacha20";

pub fn main() {
    let result = cc20Add32(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
