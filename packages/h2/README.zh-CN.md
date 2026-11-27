[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# h2

Zero-allocation RFC 7541 HTTP/2 HPACK static table (61 entries), N-bit prefixed integer codec, and indexed header encoder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:h2` ([https://github.com/hyperium/h2](https://github.com/hyperium/h2))（原库名 `h2` → CP 包名 `h2`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `h2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add h2
```

```cp
import {
    StaticEntry,
    IntDecodeResult,
    hpackEqBytes,
    hpackEntry,
    staticTableGet,
    staticTableFind,
    staticTableFindName,
    encodePrefixedInt,
} from "h2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StaticEntry` | `class` | 字段: `name: string, value: string, valid: bool` |
| `IntDecodeResult` | `class` | 字段: `value: u64, consumed: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hpackEqBytes` | `(a: &[]u8, b: &[]u8): bool` | 调用 `hpackEqBytes` 并返回 `bool`。 |
| `hpackEntry` | `(n: string, v: string): StaticEntry` | 调用 `hpackEntry` 并返回 `StaticEntry`。 |
| `staticTableGet` | `(index: usize): StaticEntry` | 调用 `staticTableGet` 并返回 `StaticEntry`。 |
| `staticTableFind` | `(name: string, value: string): usize` | 调用 `staticTableFind` 并返回 `usize`。 |
| `staticTableFindName` | `(name: string): usize` | 调用 `staticTableFindName` 并返回 `usize`。 |
| `encodePrefixedInt` | `(val: u64, prefix_bits: u8, first_byte_mask: u8, out: []u8): usize` | 通过 `encodePrefixedInt` 编码或格式化输出，返回 `usize`。 |
| `decodePrefixedInt` | `(data: &[]u8, prefix_bits: u8): IntDecodeResult` | 通过 `decodePrefixedInt` 解析输入数据，返回 `IntDecodeResult`。 |
| `encodeIndexedHeader` | `(index: usize, out: []u8): usize` | 通过 `encodeIndexedHeader` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { hpackEntry } from "h2";

pub fn main() {
    let result = hpackEntry("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
