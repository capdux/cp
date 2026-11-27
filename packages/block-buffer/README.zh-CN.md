[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# block-buffer

Zero-allocation fixed-size block streaming buffers (64/128-byte) and cryptographic padding primitives (Merkle-Damgard BE/LE, PKCS#7, ISO 7816, Zero) for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:block-buffer` ([https://github.com/RustCrypto/utils](https://github.com/RustCrypto/utils)) / `crates.io:crypto-common` ([https://github.com/RustCrypto/traits](https://github.com/RustCrypto/traits))

| 元信息 | 值 |
| --- | --- |
| 包名 | `block-buffer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add block-buffer
```

```cp
import { BlockBuffer64, BlockBuffer128, bbWriteBe64, bbWriteLe64 } from "block-buffer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BlockBuffer64` | `class` | 字段: `buf: [64]u8, pos: usize, total_bytes: u64` · 方法: `new, remaining, getPos, reset, feed, padMdBe64, +4 项` |
| `BlockBuffer128` | `class` | 字段: `buf: [128]u8, pos: usize, total_bytes: u64` · 方法: `new, remaining, getPos, reset, feed, padMdBe64, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bbWriteBe64` | `(out: []u8, offset: usize, val: u64)` | 就地执行 `bbWriteBe64` 操作。 |
| `bbWriteLe64` | `(out: []u8, offset: usize, val: u64)` | 就地执行 `bbWriteLe64` 操作。 |

## 最小调用示例

```cp
import { bbWriteBe64 } from "block-buffer";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    bbWriteBe64(out_buf[0..256], 0, 0);
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
