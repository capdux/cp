[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ngeohash

Zero-allocation Geohash Base32 spatial coordinate encoder, decoder, and bounding box library for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ngeohash` ([https://github.com/sunng87/node-geohash](https://github.com/sunng87/node-geohash))（原库名 `ngeohash` → CP 包名 `ngeohash`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ngeohash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ngeohash
```

```cp
import {
    GeoBounds,
    GeoPoint,
    ghEncodeChar,
    ghDecodeChar,
    isValidGeohash,
    encode,
    decodeBounds,
    decode,
} from "ngeohash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GeoBounds` | `class` | 字段: `min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64, valid: bool` |
| `GeoPoint` | `class` | 字段: `lat: f64, lon: f64, lat_err: f64, lon_err: f64, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ghEncodeChar` | `(v: u8): u8` | 调用 `ghEncodeChar` 并返回 `u8`。 |
| `ghDecodeChar` | `(c: u8): u8` | 调用 `ghDecodeChar` 并返回 `u8`。 |
| `isValidGeohash` | `(hash: string): bool` | 执行 `isValidGeohash` 条件判定，返回 `bool`。 |
| `encode` | `(lat: f64, lon: f64, precision: usize, out: []u8): usize` | 通过 `encode` 编码或格式化输出，返回 `usize`。 |
| `decodeBounds` | `(hash: string): GeoBounds` | 通过 `decodeBounds` 解析输入数据，返回 `GeoBounds`。 |
| `decode` | `(hash: string): GeoPoint` | 通过 `decode` 解析输入数据，返回 `GeoPoint`。 |

## 最小调用示例

```cp
import { isValidGeohash } from "ngeohash";

pub fn main() {
    let result = isValidGeohash("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
