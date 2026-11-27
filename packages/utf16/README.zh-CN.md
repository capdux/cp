[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# utf16

Zero-heap UTF-8 <-> UTF-16 / UTF-16LE codec with full surrogate pair support, lone-surrogate rejection, BOM detection, and UTF-16 unit counting.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:utf16` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:widestring` ([https://github.com/starkat99/widestring-rs](https://github.com/starkat99/widestring-rs))（原库名 `utf16` → CP 包名 `utf16`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `utf16` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add utf16
```

```cp
import {
    detectBom,
    decodeUtf8Next,
    encodeUtf8Scalar,
    countUtf16Units,
    utf8ToUtf16,
    utf16ToUtf8,
    utf8ToUtf16LeBytes,
    utf16LeBytesToUtf8,
} from "utf16";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `detectBom` | `(data: &[]u8): u8` | 调用 `detectBom` 并返回 `u8`。 |
| `decodeUtf8Next` | `(s: string, pos: usize, out_cp: &mut u32): i64` | 通过 `decodeUtf8Next` 解析输入数据，返回 `i64`。 |
| `encodeUtf8Scalar` | `(cp: u32, out: []u8, pos: usize): i64` | 通过 `encodeUtf8Scalar` 编码或格式化输出，返回 `i64`。 |
| `countUtf16Units` | `(utf8: string): i64` | 调用 `countUtf16Units` 并返回 `i64`。 |
| `utf8ToUtf16` | `(utf8: string, out: []u16): i64` | 调用 `utf8ToUtf16` 并返回 `i64`。 |
| `utf16ToUtf8` | `(utf16: &[]u16, out: []u8): i64` | 调用 `utf16ToUtf8` 并返回 `i64`。 |
| `utf8ToUtf16LeBytes` | `(utf8: string, out: []u8): i64` | 调用 `utf8ToUtf16LeBytes` 并返回 `i64`。 |
| `utf16LeBytesToUtf8` | `(bytes: &[]u8, out: []u8): i64` | 调用 `utf16LeBytesToUtf8` 并返回 `i64`。 |

## 最小调用示例

```cp
import { decodeUtf8Next } from "utf16";

pub fn main() {
    let result = decodeUtf8Next("example", 0, /* out_cp: &mut u32 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
