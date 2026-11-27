[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# multipart

Zero-heap RFC 7578 multipart/form-data streaming reader, writer, and boundary extractor built on mediatype and unicase (clean-room port of Rust multipart/multer)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:multipart` ([https://github.com/abonander/multipart](https://github.com/abonander/multipart))

| 元信息 | 值 |
| --- | --- |
| 包名 | `multipart` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add multipart
```

```cp
import {
    MultipartPart,
    MultipartReader,
    MultipartWriter,
    mpartIsSpace,
    mpartMatchAt,
    mpartIsBoundaryAt,
    mpartFindFirstBoundary,
    mpartFindNextBoundary,
} from "multipart";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MultipartPart` | `class` | 字段: `name: string, filename: string, content_type: string, data: string, is_file: bool` |
| `MultipartReader` | `class` | 字段: `boundary: string, body: string, cursor: usize, finished: bool` · 方法: `new, nextPart, countParts` |
| `MultipartWriter` | `class` | 字段: `boundary: string` · 方法: `new, writeField, writeFile, finish` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mpartIsSpace` | `(c: u8): bool` | 调用 `mpartIsSpace` 并返回 `bool`。 |
| `mpartMatchAt` | `(haystack: string, pos: usize, needle: string): bool` | 调用 `mpartMatchAt` 并返回 `bool`。 |
| `mpartIsBoundaryAt` | `(body: string, pos: usize, boundary: string): bool` | 调用 `mpartIsBoundaryAt` 并返回 `bool`。 |
| `mpartFindFirstBoundary` | `(body: string, boundary: string): usize` | 调用 `mpartFindFirstBoundary` 并返回 `usize`。 |
| `mpartFindNextBoundary` | `(body: string, start: usize, boundary: string, data_end_out: &mut usize, next_bnd_out: &mut usize): bool` | 调用 `mpartFindNextBoundary` 并返回 `bool`。 |
| `mpartSkipLineEnd` | `(body: string, pos: &mut usize): bool` | 调用 `mpartSkipLineEnd` 并返回 `bool`。 |
| `mpartParseDisposition` | `(val: string, part: &mut MultipartPart)` | 就地执行 `mpartParseDisposition` 操作。 |
| `extractBoundary` | `(content_type: string): string?` | 调用 `extractBoundary` 并返回 `string?`。 |
| `formatContentTypeHeader` | `(boundary: string, out: []u8): usize` | 通过 `formatContentTypeHeader` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatContentTypeHeader } from "multipart";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatContentTypeHeader("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
