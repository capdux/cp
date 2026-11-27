[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# seahash

Zero-heap pure-CP 4-lane parallel SeaHash 64-bit fast non-cryptographic hash function (crates.io seahash equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:seahash` ([https://github.com/ticki/tfs](https://github.com/ticki/tfs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `seahash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add seahash
```

```cp
import {
    SeaHasher,
    seaU64,
    seaMul64,
    seahashDiffuse,
    seahashBytes,
    seahashSeeded,
} from "seahash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SeaHasher` | `class` | 字段: `a: u64, b: u64, c: u64, d: u64, buf: [8]u8, buf_len: usize, +1 项` · 方法: `new, withSeeds, writeU64, writeBytes, writeStr, finish` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `seaU64` | `(hi: u32, lo: u32): u64` | 调用 `seaU64` 并返回 `u64`。 |
| `seaMul64` | `(a: u64, b: u64): u64` | 调用 `seaMul64` 并返回 `u64`。 |
| `seahashDiffuse` | `(x_in: u64): u64` | 调用 `seahashDiffuse` 并返回 `u64`。 |
| `seahashBytes` | `(buf: []u8): u64` | 调用 `seahashBytes` 并返回 `u64`。 |
| `seahashSeeded` | `(buf: []u8, k1: u64, k2: u64, k3: u64, k4: u64): u64` | 调用 `seahashSeeded` 并返回 `u64`。 |

## 最小调用示例

```cp
import { seaU64 } from "seahash";

pub fn main() {
    let result = seaU64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
