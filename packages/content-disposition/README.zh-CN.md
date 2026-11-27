[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# content-disposition

Zero-allocation RFC 6266 / RFC 5987 HTTP Content-Disposition header parser, formatter, filename* decoder, and filename sanitizer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:content-disposition` ([https://github.com/jshttp/content-disposition](https://github.com/jshttp/content-disposition))

| 元信息 | 值 |
| --- | --- |
| 包名 | `content-disposition` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add content-disposition
```

```cp
import {
    ContentDisposition,
    cdspIsOws,
    cdspIsTokenChar,
    cdspSkipOws,
    cdspHasNonAscii,
    cdspWriteByte,
    cdspWriteStr,
    cdspWriteAsciiFallback,
} from "content-disposition";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ContentDisposition` | `class` | 字段: `valid: bool, disp_type: string, filename: string, filename_star: string` · 方法: `invalid, isAttachment, isInline, hasFilename` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cdspIsOws` | `(b: u8): bool` | 调用 `cdspIsOws` 并返回 `bool`。 |
| `cdspIsTokenChar` | `(b: u8): bool` | 调用 `cdspIsTokenChar` 并返回 `bool`。 |
| `cdspSkipOws` | `(s: string, start: usize): usize` | 调用 `cdspSkipOws` 并返回 `usize`。 |
| `cdspHasNonAscii` | `(s: string): bool` | 调用 `cdspHasNonAscii` 并返回 `bool`。 |
| `cdspWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `cdspWriteByte` 并返回 `usize`。 |
| `cdspWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `cdspWriteStr` 并返回 `usize`。 |
| `cdspWriteAsciiFallback` | `(out: []u8, pos: usize, s: string): usize` | 调用 `cdspWriteAsciiFallback` 并返回 `usize`。 |
| `cdspRfc5987Set` | `(): AsciiSet` | 调用 `cdspRfc5987Set` 并返回 `AsciiSet`。 |
| `cdspIsControl` | `(b: u8): bool` | 调用 `cdspIsControl` 并返回 `bool`。 |
| `parseContentDisposition` | `(header: string): ContentDisposition` | 通过 `parseContentDisposition` 解析输入数据，返回 `ContentDisposition`。 |
| `sanitizeFilename` | `(raw: string, out: []u8): usize` | 调用 `sanitizeFilename` 并返回 `usize`。 |
| `decodeFilenameStar` | `(raw_star: string, out: []u8): usize` | 通过 `decodeFilenameStar` 解析输入数据，返回 `usize`。 |
| `formatContentDisposition` | `(disp_type: string, filename: string, out: []u8): usize` | 通过 `formatContentDisposition` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseContentDisposition } from "content-disposition";

pub fn main() {
    let result = parseContentDisposition("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
