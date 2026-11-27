[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @opentelemetry/core

Zero-allocation W3C Baggage (OpenTelemetry) HTTP header parser, percent-codec, and serializer in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@opentelemetry/core` ([https://github.com/open-telemetry/opentelemetry-js](https://github.com/open-telemetry/opentelemetry-js))（原库名 `@opentelemetry/core` → CP 包名 `@opentelemetry/core`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@opentelemetry/core` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @opentelemetry/core
```

```cp
import {
    BaggageMember,
    isSpace,
    isTchar,
    isValidKey,
    trimSlice,
    strEq,
    parseSingleMember,
    parseBaggage,
} from "@opentelemetry/core";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BaggageMember` | `class` | 字段: `key: string, value: string, properties: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSpace` | `(b: u8): bool` | 执行 `isSpace` 条件判定，返回 `bool`。 |
| `isTchar` | `(b: u8): bool` | 执行 `isTchar` 条件判定，返回 `bool`。 |
| `isValidKey` | `(key: string): bool` | 执行 `isValidKey` 条件判定，返回 `bool`。 |
| `trimSlice` | `(s: string): string` | 调用 `trimSlice` 并返回 `string`。 |
| `strEq` | `(a: string, b: string): bool` | 调用 `strEq` 并返回 `bool`。 |
| `parseSingleMember` | `(seg: string, out: &mut BaggageMember): bool` | 通过 `parseSingleMember` 解析输入数据，返回 `bool`。 |
| `parseBaggage` | `(header: string, out: []BaggageMember): usize` | 通过 `parseBaggage` 解析输入数据，返回 `usize`。 |
| `countMembers` | `(header: string): usize` | 调用 `countMembers` 并返回 `usize`。 |
| `hexVal` | `(b: u8): i32` | 调用 `hexVal` 并返回 `i32`。 |
| `percentDecode` | `(src: string, out: []u8, out_len: &mut usize): bool` | 调用 `percentDecode` 并返回 `bool`。 |
| `getDecodedValue` | `(header: string, target_key: string, out: []u8, out_len: &mut usize): bool` | 查询或读取目标数据，返回 `bool`。 |
| `hexUpper` | `(nibble: u8): u8` | 调用 `hexUpper` 并返回 `u8`。 |
| `needsPercentEncode` | `(b: u8): bool` | 调用 `needsPercentEncode` 并返回 `bool`。 |
| `formatBaggage` | `(members: &[]BaggageMember, out: []u8): usize` | 通过 `formatBaggage` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidKey } from "@opentelemetry/core";

pub fn main() {
    let result = isValidKey("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
