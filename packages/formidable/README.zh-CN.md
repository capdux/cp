[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# formidable

Zero-heap pure CP multipart/form-data parser for fields and file uploads (clean-room MIT implementation of npm formidable v3)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:formidable` ([https://github.com/node-formidable/formidable](https://github.com/node-formidable/formidable))

| 元信息 | 值 |
| --- | --- |
| 包名 | `formidable` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add formidable
```

```cp
import {
    FormidableOptions,
    FormField,
    FormFile,
    ParsedForm,
    frmIsOws,
    frmIsTokenChar,
    frmTrimOws,
    frmSkipOws,
} from "formidable";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FormidableOptions` | `class` | 字段: `max_files: usize, max_fields: usize, max_file_size: u64, allow_empty_files: bool` · 方法: `defaults` |
| `FormField` | `class` | 字段: `name: string, value: string` |
| `FormFile` | `class` | 字段: `field_name: string, original_filename: string, mime_type: string, body_start: usize, body_len: usize` |
| `ParsedForm` | `class` | 字段: `fields: [16]FormField, field_count: usize, files: [16]FormFile, file_count: usize, valid: bool, err_code: u8` · 方法: `empty, fail, getField, getFile` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `frmIsOws` | `(b: u8): bool` | 调用 `frmIsOws` 并返回 `bool`。 |
| `frmIsTokenChar` | `(b: u8): bool` | 调用 `frmIsTokenChar` 并返回 `bool`。 |
| `frmTrimOws` | `(s: string): string` | 调用 `frmTrimOws` 并返回 `string`。 |
| `frmSkipOws` | `(s: string, start: usize): usize` | 调用 `frmSkipOws` 并返回 `usize`。 |
| `frmIsBoundaryTerminator` | `(body: string, after_pos: usize): bool` | 调用 `frmIsBoundaryTerminator` 并返回 `bool`。 |
| `frmFindFirstBoundary` | `(body: string, boundary: string): usize` | 调用 `frmFindFirstBoundary` 并返回 `usize`。 |
| `frmFindNextBoundary` | `(body: string, start: usize, boundary: string): usize` | 调用 `frmFindNextBoundary` 并返回 `usize`。 |
| `frmExtractDispositionName` | `(header: string): string` | 调用 `frmExtractDispositionName` 并返回 `string`。 |
| `frmHasFilenameParam` | `(header: string): bool` | 调用 `frmHasFilenameParam` 并返回 `bool`。 |
| `parseMultipartForm` | `(content_type: string, body: string, opts: FormidableOptions): ParsedForm` | 通过 `parseMultipartForm` 解析输入数据，返回 `ParsedForm`。 |

## 最小调用示例

```cp
import { parseMultipartForm } from "formidable";

pub fn main() {
    let result = parseMultipartForm("example", "example", /* opts: FormidableOptions */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
