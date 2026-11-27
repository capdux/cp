[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# haversine

Zero-allocation WGS-84 spherical Haversine great-circle distance, initial bearing, midpoint, and destination point calculator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:haversine` ([https://github.com/njj/haversine](https://github.com/njj/haversine)) / `crates.io:haversine` ([https://github.com/gavinuhma/haversine](https://github.com/gavinuhma/haversine))（原库名 `haversine` → CP 包名 `haversine`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `haversine` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add haversine
```

```cp
import {
    LatLon,
    hgAbs,
    hgClamp,
    hgSqrt,
    hgWrapPi,
    hgSin,
    hgCos,
    hgAtan,
} from "haversine";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LatLon` | `class` | 字段: `lat: f64, lon: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hgAbs` | `(x: f64): f64` | 调用 `hgAbs` 并返回 `f64`。 |
| `hgClamp` | `(x: f64, lo: f64, hi: f64): f64` | 调用 `hgClamp` 并返回 `f64`。 |
| `hgSqrt` | `(x: f64): f64` | 调用 `hgSqrt` 并返回 `f64`。 |
| `hgWrapPi` | `(rad: f64): f64` | 调用 `hgWrapPi` 并返回 `f64`。 |
| `hgSin` | `(rad: f64): f64` | 调用 `hgSin` 并返回 `f64`。 |
| `hgCos` | `(rad: f64): f64` | 调用 `hgCos` 并返回 `f64`。 |
| `hgAtan` | `(z: f64): f64` | 调用 `hgAtan` 并返回 `f64`。 |
| `hgAtan2` | `(y: f64, x: f64): f64` | 调用 `hgAtan2` 并返回 `f64`。 |
| `hgAsin` | `(x: f64): f64` | 调用 `hgAsin` 并返回 `f64`。 |
| `hgDegToRad` | `(deg: f64): f64` | 调用 `hgDegToRad` 并返回 `f64`。 |
| `hgRadToDeg` | `(rad: f64): f64` | 调用 `hgRadToDeg` 并返回 `f64`。 |
| `hgNormLon` | `(lon: f64): f64` | 调用 `hgNormLon` 并返回 `f64`。 |
| `isValidCoord` | `(p: LatLon): bool` | 执行 `isValidCoord` 条件判定，返回 `bool`。 |
| `distanceMeters` | `(a: LatLon, b: LatLon): f64` | 调用 `distanceMeters` 并返回 `f64`。 |
| `distanceKm` | `(a: LatLon, b: LatLon): f64` | 调用 `distanceKm` 并返回 `f64`。 |
| `distanceMiles` | `(a: LatLon, b: LatLon): f64` | 调用 `distanceMiles` 并返回 `f64`。 |
| `initialBearing` | `(a: LatLon, b: LatLon): f64` | 创建或初始化实例，返回 `f64`。 |
| `midpoint` | `(a: LatLon, b: LatLon): LatLon` | 调用 `midpoint` 并返回 `LatLon`。 |
| `destinationPoint` | `(start: LatLon, bearing_deg: f64, distance_m: f64): LatLon` | 调用 `destinationPoint` 并返回 `LatLon`。 |

## 最小调用示例

```cp
import { isValidCoord } from "haversine";

pub fn main() {
    let result = isValidCoord(/* p: LatLon */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
