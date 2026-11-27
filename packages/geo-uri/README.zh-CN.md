[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# geo-uri

Zero-allocation RFC 5870 geo: URI parser, coordinate validator, and formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:geo-uri` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `geo-uri` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add geo-uri
```

```cp
import {
    GeoUri,
    guParseF64,
    guWriteRaw,
    guWriteU64,
    guWriteF64,
    isValidLatLng,
    parseGeoUri,
    formatGeoUri,
} from "geo-uri";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GeoUri` | `class` | 字段: `lat: f64, lng: f64, has_alt: bool, alt: f64, has_uncertainty: bool, uncertainty: f64, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `guParseF64` | `(s: &[]u8, out: &mut f64): bool` | 调用 `guParseF64` 并返回 `bool`。 |
| `guWriteRaw` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `guWriteRaw` 并返回 `bool`。 |
| `guWriteU64` | `(out: []u8, pos: &mut usize, val: u64): bool` | 调用 `guWriteU64` 并返回 `bool`。 |
| `guWriteF64` | `(out: []u8, pos: &mut usize, val: f64): bool` | 调用 `guWriteF64` 并返回 `bool`。 |
| `isValidLatLng` | `(lat: f64, lng: f64): bool` | 执行 `isValidLatLng` 条件判定，返回 `bool`。 |
| `parseGeoUri` | `(uri: string): GeoUri` | 通过 `parseGeoUri` 解析输入数据，返回 `GeoUri`。 |
| `formatGeoUri` | `(lat: f64, lng: f64, has_alt: bool, alt: f64, has_u: bool, u: f64, out: []u8): usize` | 通过 `formatGeoUri` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseGeoUri } from "geo-uri";

pub fn main() {
    let result = parseGeoUri("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
