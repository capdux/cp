[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# phonenumber

Zero-heap RFC 3966 tel: URI parser, E.164 international phone number normalizer/validator, and URI formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:phonenumber` ([https://github.com/whisperfish/rust-phonenumber](https://github.com/whisperfish/rust-phonenumber))（原库名 `phonenumber` → CP 包名 `phonenumber`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `phonenumber` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add phonenumber
```

```cp
import {
    TelUri,
    isVisualSep,
    isWs,
    isValidE164,
    normalizeE164,
    parseTelUri,
    appendSlice,
    formatTelUri,
} from "phonenumber";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TelUri` | `class` | 字段: `number: string, is_global: bool, ext: string, phone_context: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isVisualSep` | `(c: u8): bool` | 执行 `isVisualSep` 条件判定，返回 `bool`。 |
| `isWs` | `(c: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `isValidE164` | `(number: string): bool` | 执行 `isValidE164` 条件判定，返回 `bool`。 |
| `normalizeE164` | `(input: string, out: []u8): usize` | 调用 `normalizeE164` 并返回 `usize`。 |
| `parseTelUri` | `(uri: string): TelUri` | 通过 `parseTelUri` 解析输入数据，返回 `TelUri`。 |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `formatTelUri` | `(e164_number: string, ext: string, out: []u8): usize` | 通过 `formatTelUri` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidE164 } from "phonenumber";

pub fn main() {
    let result = isValidE164("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
