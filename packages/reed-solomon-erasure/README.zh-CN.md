[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# reed-solomon-erasure

Zero-allocation GF(2^8) Cauchy Reed-Solomon erasure coding and FEC reconstruction engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:reed-solomon-erasure` ([https://github.com/darrenldl/reed-solomon-erasure](https://github.com/darrenldl/reed-solomon-erasure))（原库名 `reed-solomon-erasure` → CP 包名 `reed-solomon-erasure`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `reed-solomon-erasure` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add reed-solomon-erasure
```

```cp
import { RsCodec, gfMul, gfPow, gfInv } from "reed-solomon-erasure";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RsCodec` | `class` | 字段: `data_count: usize, parity_count: usize, parity_matrix: [128]u8` · 方法: `init, dataShards, parityShards, totalShards, encodeByte, verifyByte, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `gfMul` | `(a: u8, b: u8): u8` | 调用 `gfMul` 并返回 `u8`。 |
| `gfPow` | `(a: u8, n: u32): u8` | 调用 `gfPow` 并返回 `u8`。 |
| `gfInv` | `(a: u8): u8` | 调用 `gfInv` 并返回 `u8`。 |

## 最小调用示例

```cp
import { gfMul } from "reed-solomon-erasure";

pub fn main() {
    let result = gfMul(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
