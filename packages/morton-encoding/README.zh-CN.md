[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# morton-encoding

Zero-allocation 2D/3D Z-Order (Morton) bit-interleaving codec and 2D Hilbert space-filling curve mapper for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:morton-encoding` ([https://github.com/morton-encoding/morton-encoding](https://github.com/morton-encoding/morton-encoding))（原库名 `morton-encoding` → CP 包名 `morton-encoding`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `morton-encoding` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add morton-encoding
```

```cp
import {
    Point2D,
    Point3D,
    splitBy1,
    compactBy1,
    encode2D,
    decode2D,
    splitBy2,
    compactBy2,
} from "morton-encoding";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Point2D` | `class` | 字段: `x: u32, y: u32` |
| `Point3D` | `class` | 字段: `x: u32, y: u32, z: u32` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `splitBy1` | `(v: u32): u64` | 调用 `splitBy1` 并返回 `u64`。 |
| `compactBy1` | `(v: u64): u32` | 调用 `compactBy1` 并返回 `u32`。 |
| `encode2D` | `(x: u32, y: u32): u64` | 通过 `encode2D` 编码或格式化输出，返回 `u64`。 |
| `decode2D` | `(code: u64): Point2D` | 通过 `decode2D` 解析输入数据，返回 `Point2D`。 |
| `splitBy2` | `(v: u32): u64` | 调用 `splitBy2` 并返回 `u64`。 |
| `compactBy2` | `(v: u64): u32` | 调用 `compactBy2` 并返回 `u32`。 |
| `encode3D` | `(x: u32, y: u32, z: u32): u64` | 通过 `encode3D` 编码或格式化输出，返回 `u64`。 |
| `decode3D` | `(code: u64): Point3D` | 通过 `decode3D` 解析输入数据，返回 `Point3D`。 |
| `clampOrder` | `(order: u32): u32` | 调用 `clampOrder` 并返回 `u32`。 |
| `hilbertRot` | `(n: u32, x_in: u32, y_in: u32, rx: u32, ry: u32): Point2D` | 调用 `hilbertRot` 并返回 `Point2D`。 |
| `hilbert2DEncode` | `(order: u32, x_in: u32, y_in: u32): u64` | 调用 `hilbert2DEncode` 并返回 `u64`。 |
| `hilbert2DDecode` | `(order: u32, d_in: u64): Point2D` | 调用 `hilbert2DDecode` 并返回 `Point2D`。 |

## 最小调用示例

```cp
import { encode2D } from "morton-encoding";

pub fn main() {
    let result = encode2D(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
