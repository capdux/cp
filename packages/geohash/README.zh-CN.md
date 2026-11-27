[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# geohash

Zero-allocation Base32 Geohash encoder, bounding-box decoder, and 8-direction neighbor finder (Rust geohash / ngeohash parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geohash` ([https://github.com/georust/geohash](https://github.com/georust/geohash))

| 元信息 | 值 |
| --- | --- |
| 包名 | `geohash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add geohash
```

```cp
import {
    GeoPoint,
    GeoBBox,
    invalidBBox,
    base32Index,
    isValid,
    encode,
    decodeBBox,
    decode,
} from "geohash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GeoPoint` | `class` | 字段: `lat: f64, lon: f64` |
| `GeoBBox` | `class` | 字段: `min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidBBox` | `(): GeoBBox` | 调用 `invalidBBox` 并返回 `GeoBBox`。 |
| `base32Index` | `(c: u8): i16` | 调用 `base32Index` 并返回 `i16`。 |
| `isValid` | `(hash: &[]u8): bool` | 执行 `isValid` 条件判定，返回 `bool`。 |
| `encode` | `(lat: f64, lon: f64, precision: usize, dst: []u8): usize` | 通过 `encode` 编码或格式化输出，返回 `usize`。 |
| `decodeBBox` | `(hash: &[]u8): GeoBBox` | 通过 `decodeBBox` 解析输入数据，返回 `GeoBBox`。 |
| `decode` | `(hash: &[]u8): GeoPoint` | 通过 `decode` 解析输入数据，返回 `GeoPoint`。 |
| `neighbor` | `(hash: &[]u8, dlat: i32, dlon: i32, dst: []u8): usize` | 调用 `neighbor` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isValid } from "geohash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValid(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
