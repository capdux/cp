[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# punycode

Zero-allocation RFC 3492 Punycode / IDNA (xn--...) encoder and decoder for internationalized domain names in CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:punycode` ([https://github.com/mathiasbynens/punycode.js](https://github.com/mathiasbynens/punycode.js)) / `crates.io:punycode` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `punycode` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add punycode
```

```cp
import {
    pcToLower,
    pcIsXnPrefix,
    isAsciiOnly,
    pcAdapt,
    pcEncodeDigit,
    pcDecodeDigit,
    pcThreshold,
    pcDecodeUtf8,
} from "punycode";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pcToLower` | `(c: u8): u8` | 调用 `pcToLower` 并返回 `u8`。 |
| `pcIsXnPrefix` | `(s: &[]u8): bool` | 调用 `pcIsXnPrefix` 并返回 `bool`。 |
| `isAsciiOnly` | `(s: string): bool` | 执行 `isAsciiOnly` 条件判定，返回 `bool`。 |
| `pcAdapt` | `(delta_in: u32, numpoints: u32, firsttime: bool): u32` | 调用 `pcAdapt` 并返回 `u32`。 |
| `pcEncodeDigit` | `(d: u32): u8` | 调用 `pcEncodeDigit` 并返回 `u8`。 |
| `pcDecodeDigit` | `(c: u8): u32` | 调用 `pcDecodeDigit` 并返回 `u32`。 |
| `pcThreshold` | `(k: u32, bias: u32): u32` | 调用 `pcThreshold` 并返回 `u32`。 |
| `pcDecodeUtf8` | `(s: &[]u8, cps: []u32): i64` | 调用 `pcDecodeUtf8` 并返回 `i64`。 |
| `pcEncodeUtf8` | `(cps: &[]u32, out: []u8): i64` | 调用 `pcEncodeUtf8` 并返回 `i64`。 |
| `encodeLabel` | `(utf8_label: string, out: []u8): i64` | 通过 `encodeLabel` 编码或格式化输出，返回 `i64`。 |
| `decodeLabel` | `(ascii_label: string, out_utf8: []u8): i64` | 通过 `decodeLabel` 解析输入数据，返回 `i64`。 |
| `pcWriteSlice` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `pcWriteSlice` 并返回 `bool`。 |
| `toAsciiDomain` | `(utf8_domain: string, out: []u8): i64` | 调用 `toAsciiDomain` 并返回 `i64`。 |
| `toUnicodeDomain` | `(ascii_domain: string, out: []u8): i64` | 调用 `toUnicodeDomain` 并返回 `i64`。 |

## 最小调用示例

```cp
import { isAsciiOnly } from "punycode";

pub fn main() {
    let result = isAsciiOnly("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
