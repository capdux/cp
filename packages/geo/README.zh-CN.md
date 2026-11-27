[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# geo

Zero-allocation spherical geometry engine: Haversine distance, initial bearing, destination point, midpoint, bounding box, and point-in-polygon (Rust geo / haversine / geolib parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geo` ([https://github.com/georust/geo](https://github.com/georust/geo))（原库名 `geo` → CP 包名 `geo`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `geo` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add geo
```

```cp
import {
    Coord,
    Bounds,
    toRad,
    toDeg,
    normalizeLon,
    sqrtF64,
    sinF64,
    cosF64,
} from "geo";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Coord` | `class` | 字段: `lat: f64, lon: f64` |
| `Bounds` | `class` | 字段: `min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `toRad` | `(deg: f64): f64` | 调用 `toRad` 并返回 `f64`。 |
| `toDeg` | `(rad: f64): f64` | 调用 `toDeg` 并返回 `f64`。 |
| `normalizeLon` | `(lon_in: f64): f64` | 调用 `normalizeLon` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `sinF64` | `(x_in: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(x: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |
| `atanF64` | `(x_in: f64): f64` | 调用 `atanF64` 并返回 `f64`。 |
| `atan2F64` | `(y: f64, x: f64): f64` | 调用 `atan2F64` 并返回 `f64`。 |
| `asinF64` | `(x: f64): f64` | 调用 `asinF64` 并返回 `f64`。 |
| `haversineMeters` | `(a: Coord, b: Coord): f64` | 调用 `haversineMeters` 并返回 `f64`。 |
| `initialBearing` | `(a: Coord, b: Coord): f64` | 创建或初始化实例，返回 `f64`。 |
| `destinationPoint` | `(origin: Coord, bearing_deg: f64, distance_m: f64): Coord` | 调用 `destinationPoint` 并返回 `Coord`。 |
| `midpoint` | `(a: Coord, b: Coord): Coord` | 调用 `midpoint` 并返回 `Coord`。 |
| `boundingBox` | `(center: Coord, radius_m: f64): Bounds` | 调用 `boundingBox` 并返回 `Bounds`。 |
| `pointInPolygon` | `(pt: Coord, poly: &[]Coord): bool` | 调用 `pointInPolygon` 并返回 `bool`。 |

## 最小调用示例

```cp
import { toRad } from "geo";

pub fn main() {
    let result = toRad(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
