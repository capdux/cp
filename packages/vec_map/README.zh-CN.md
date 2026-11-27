[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# vec_map

Zero-allocation O(1) direct-indexed sparse integer-key map (0..127) with 128-bit presence bitmap in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vec_map` ([https://github.com/contain-rs/vec-map](https://github.com/contain-rs/vec-map), `MIT OR Apache-2.0`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `vec_map` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add vec_map
```

```cp
import { VecMapI64, vmapMask, vmapHasBit } from "vec_map";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `VecMapI64` | `class` | 字段: `slots: [128]i64, bits_lo: u64, bits_hi: u64, count: usize` · 方法: `new, len, isEmpty, containsKey, get, insert, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `vmapMask` | `(bit: usize): u64` | 调用 `vmapMask` 并返回 `u64`。 |
| `vmapHasBit` | `(bits_lo: u64, bits_hi: u64, k: usize): bool` | 调用 `vmapHasBit` 并返回 `bool`。 |

## 最小调用示例

```cp
import { vmapMask } from "vec_map";

pub fn main() {
    let result = vmapMask(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
