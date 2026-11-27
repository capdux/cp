[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# base62

Zero-heap URL-safe Base62 (0-9A-Za-z) u64 and arbitrary byte-slice encoder/decoder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:base62` ([https://github.com/fbernier/base62](https://github.com/fbernier/base62))

| 元信息 | 值 |
| --- | --- |
| 包名 | `base62` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add base62
```

```cp
import {
    DecodeU64Result,
    charToVal,
    isValidBase62,
    encodeU64,
    encodeU64Padded,
    decodeU64,
    encodeBytes,
    decodeBytes,
} from "base62";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DecodeU64Result` | `class` | 字段: `value: u64, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `charToVal` | `(c: u8): i32` | 调用 `charToVal` 并返回 `i32`。 |
| `isValidBase62` | `(src: &[]u8): bool` | 执行 `isValidBase62` 条件判定，返回 `bool`。 |
| `encodeU64` | `(value: u64, dst: []u8): usize` | 通过 `encodeU64` 编码或格式化输出，返回 `usize`。 |
| `encodeU64Padded` | `(value: u64, width: usize, dst: []u8): usize` | 通过 `encodeU64Padded` 编码或格式化输出，返回 `usize`。 |
| `decodeU64` | `(src: &[]u8): DecodeU64Result` | 通过 `decodeU64` 解析输入数据，返回 `DecodeU64Result`。 |
| `encodeBytes` | `(src: &[]u8, dst: []u8): i64` | 通过 `encodeBytes` 编码或格式化输出，返回 `i64`。 |
| `decodeBytes` | `(src: &[]u8, dst: []u8): i64` | 通过 `decodeBytes` 解析输入数据，返回 `i64`。 |

## 最小调用示例

```cp
import { isValidBase62 } from "base62";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidBase62(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
