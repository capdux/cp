[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lz-string

Zero-heap LZW byte/string compression with Base64 and URI-safe encoding for CP (clean-room MIT implementation of npm lz-string).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lz-string` ([https://github.com/pieroxy/lz-string](https://github.com/pieroxy/lz-string))（原库名 `lz-string` → CP 包名 `lz-string`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `lz-string` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add lz-string
```

```cp
import {
    lzsHash,
    lzsClearTable,
    lzsFind,
    lzsInsert,
    compressBytes,
    decompressBytes,
    lzsEncode64,
    lzsDecodeChar,
} from "lz-string";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lzsHash` | `(key: u32): usize` | 调用 `lzsHash` 并返回 `usize`。 |
| `lzsClearTable` | `(ht_key: []u32)` | 就地执行 `lzsClearTable` 操作。 |
| `lzsFind` | `(ht_key: &[]u32, ht_val: &[]u16, key: u32): i32` | 调用 `lzsFind` 并返回 `i32`。 |
| `lzsInsert` | `(ht_key: []u32, ht_val: []u16, key: u32, code: u16)` | 就地执行 `lzsInsert` 操作。 |
| `compressBytes` | `(input: &[]u8, out: []u8): usize` | 调用 `compressBytes` 并返回 `usize`。 |
| `decompressBytes` | `(compressed: &[]u8, out: []u8): i64` | 调用 `decompressBytes` 并返回 `i64`。 |
| `lzsEncode64` | `(raw: &[]u8, alphabet: string, pad: bool, out: []u8): usize` | 调用 `lzsEncode64` 并返回 `usize`。 |
| `lzsDecodeChar` | `(ch: u8, uri_mode: bool): i32` | 调用 `lzsDecodeChar` 并返回 `i32`。 |
| `lzsDecode64` | `(text: string, uri_mode: bool, out: []u8): i64` | 调用 `lzsDecode64` 并返回 `i64`。 |
| `compressToBase64` | `(input: string, out: []u8): usize` | 调用 `compressToBase64` 并返回 `usize`。 |
| `decompressFromBase64` | `(b64: string, out: []u8): i64` | 调用 `decompressFromBase64` 并返回 `i64`。 |
| `compressToEncodedURIComponent` | `(input: string, out: []u8): usize` | 调用 `compressToEncodedURIComponent` 并返回 `usize`。 |
| `decompressFromEncodedURIComponent` | `(uri: string, out: []u8): i64` | 调用 `decompressFromEncodedURIComponent` 并返回 `i64`。 |

## 最小调用示例

```cp
import { lzsDecode64 } from "lz-string";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lzsDecode64("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
