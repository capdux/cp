[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# error-ex

Zero-heap structured error and context formatter with template interpolation

| 元信息 | 值 |
| --- | --- |
| 包名 | `error-ex` |
| 版本 | `1.3.2` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add error-ex
```

```cp
import {
    ErrorField,
    StructuredError,
    erexWriteByte,
    erexWriteStr,
    erexWriteI32,
    formatStructuredError,
    erexMatchesPlaceholder,
    formatErrorTemplate,
} from "error-ex";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ErrorField` | `class` | 字段: `key: string, value: string, line_mode: bool` |
| `StructuredError` | `class` | 字段: `name: string, code: i32, message: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `erexWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `erexWriteByte` 并返回 `usize`。 |
| `erexWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `erexWriteStr` 并返回 `usize`。 |
| `erexWriteI32` | `(out: []u8, pos: usize, val: i32): usize` | 调用 `erexWriteI32` 并返回 `usize`。 |
| `formatStructuredError` | `(e: StructuredError, fields: &[]ErrorField, out: []u8): usize` | 通过 `formatStructuredError` 编码或格式化输出，返回 `usize`。 |
| `erexMatchesPlaceholder` | `(tmpl: string, start: usize, key: string): bool` | 调用 `erexMatchesPlaceholder` 并返回 `bool`。 |
| `formatErrorTemplate` | `(tmpl: string, key: string, val: string, out: []u8): usize` | 通过 `formatErrorTemplate` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatErrorTemplate } from "error-ex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatErrorTemplate("example", "example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
