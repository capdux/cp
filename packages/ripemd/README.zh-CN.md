[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ripemd

Zero-heap pure-CP RIPEMD-160 160-bit cryptographic hash function (crates.io ripemd equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ripemd` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `ripemd` → CP 包名 `ripemd`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ripemd` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ripemd
```

```cp
import {
    Ripemd160,
    rmdAdd,
    rmdAdd3,
    rmdAdd4,
    rmdRol,
    rmdNot,
    rmdFunc,
    rmdKl,
} from "ripemd";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Ripemd160` | `class` | 字段: `state: [5]u32, buf: [64]u8, buf_len: usize, total_len: u64` · 方法: `new, reset, update, updateStr, finalize, finalizeHex` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rmdAdd` | `(a: u32, b: u32): u32` | 调用 `rmdAdd` 并返回 `u32`。 |
| `rmdAdd3` | `(a: u32, b: u32, c: u32): u32` | 调用 `rmdAdd3` 并返回 `u32`。 |
| `rmdAdd4` | `(a: u32, b: u32, c: u32, d: u32): u32` | 调用 `rmdAdd4` 并返回 `u32`。 |
| `rmdRol` | `(x: u32, n: u32): u32` | 调用 `rmdRol` 并返回 `u32`。 |
| `rmdNot` | `(x: u32): u32` | 调用 `rmdNot` 并返回 `u32`。 |
| `rmdFunc` | `(round: usize, x: u32, y: u32, z: u32): u32` | 调用 `rmdFunc` 并返回 `u32`。 |
| `rmdKl` | `(round: usize): u32` | 调用 `rmdKl` 并返回 `u32`。 |
| `rmdKr` | `(round: usize): u32` | 调用 `rmdKr` 并返回 `u32`。 |
| `rmdCompress` | `(state: [5]u32, block: []u8): [5]u32` | 调用 `rmdCompress` 并返回 `[5]u32`。 |
| `ripemd160Bytes` | `(data: []u8, out20: []u8)` | 就地执行 `ripemd160Bytes` 操作。 |
| `ripemd160Hex` | `(s: string, out40: []u8): usize` | 调用 `ripemd160Hex` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ripemd160Hex } from "ripemd";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ripemd160Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
