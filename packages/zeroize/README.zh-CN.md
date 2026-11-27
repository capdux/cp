[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zeroize

Zero-heap secure secret memory wiping and ZeroizingBuf32/ZeroizingBuf64 key containers (crates.io zeroize equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zeroize` ([https://github.com/RustCrypto/utils](https://github.com/RustCrypto/utils))

| 元信息 | 值 |
| --- | --- |
| 包名 | `zeroize` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add zeroize
```

```cp
import {
    ZeroizingKey32,
    ZeroizingBuf64,
    zeroizeBytes,
    zeroizeU32,
    zeroizeU64,
    isAllZeroBytes,
} from "zeroize";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ZeroizingKey32` | `class` | 字段: `bytes: [32]u8, active: bool` · 方法: `fromSlice, zeroize, isZeroed` |
| `ZeroizingBuf64` | `class` | 字段: `bytes: [64]u8, len: usize, active: bool` · 方法: `fromSlice, zeroize, isZeroed` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `zeroizeBytes` | `(buf: []u8): usize` | 调用 `zeroizeBytes` 并返回 `usize`。 |
| `zeroizeU32` | `(buf: []u32): usize` | 调用 `zeroizeU32` 并返回 `usize`。 |
| `zeroizeU64` | `(buf: []u64): usize` | 调用 `zeroizeU64` 并返回 `usize`。 |
| `isAllZeroBytes` | `(buf: []u8): bool` | 执行 `isAllZeroBytes` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isAllZeroBytes } from "zeroize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isAllZeroBytes(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
