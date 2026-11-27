[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# s2

Zero-heap Google S2 spherical geometry: 64-bit S2CellId face/level/parent/child/contains, LatLng validation, and spherical distance (crates.io s2 equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:s2` ([https://github.com/roydbt/s2](https://github.com/roydbt/s2))

| 元信息 | 值 |
| --- | --- |
| 包名 | `s2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add s2
```

```cp
import {
    LatLng,
    S2CellId,
    s2TrailingZeros,
    s2MaskAbove,
    s2Sin,
    s2Cos,
    s2Sqrt,
    s2Asin,
} from "s2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LatLng` | `class` | 字段: `lat_deg: f64, lng_deg: f64` · 方法: `fromDegrees, isValid, latRad, lngRad` |
| `S2CellId` | `class` | 字段: `id: u64` · 方法: `fromFace, fromFacePosLevel, face, lsb, isValid, level, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `s2TrailingZeros` | `(v: u64): u8` | 调用 `s2TrailingZeros` 并返回 `u8`。 |
| `s2MaskAbove` | `(lsb_val: u64): u64` | 调用 `s2MaskAbove` 并返回 `u64`。 |
| `s2Sin` | `(x_in: f64): f64` | 调用 `s2Sin` 并返回 `f64`。 |
| `s2Cos` | `(x_in: f64): f64` | 调用 `s2Cos` 并返回 `f64`。 |
| `s2Sqrt` | `(x: f64): f64` | 调用 `s2Sqrt` 并返回 `f64`。 |
| `s2Asin` | `(x: f64): f64` | 调用 `s2Asin` 并返回 `f64`。 |
| `haversineMeters` | `(a: LatLng, b: LatLng): f64` | 调用 `haversineMeters` 并返回 `f64`。 |

## 最小调用示例

```cp
import { s2TrailingZeros } from "s2";

pub fn main() {
    let result = s2TrailingZeros(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
