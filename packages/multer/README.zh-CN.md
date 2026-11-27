[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# multer

Zero-heap pure CP multipart/form-data boundary extractor and upload policy validator (clean-room MIT implementation of npm multer v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:multer` ([https://github.com/expressjs/multer](https://github.com/expressjs/multer))

| 元信息 | 值 |
| --- | --- |
| 包名 | `multer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add multer
```

```cp
import { MulterLimits, UploadedFile, MulterValidator, extractMultipartBoundary } from "multer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MulterLimits` | `class` | 字段: `file_size: u64, files: usize, fields: usize, field_size: u64` · 方法: `defaults` |
| `UploadedFile` | `class` | 字段: `field_name: string, original_name: string, mime_type: string, size: u64` |
| `MulterValidator` | `class` | 字段: `limits: MulterLimits, files: [16]UploadedFile, file_count: usize, field_count: usize, allowed_fields: [16]string, allowed_count: usize` · 方法: `new, allowField, isFieldAllowed, validateContentType, acceptFile, acceptField, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `extractMultipartBoundary` | `(content_type: string): string?` | 调用 `extractMultipartBoundary` 并返回 `string?`。 |

## 最小调用示例

```cp
import { extractMultipartBoundary } from "multer";

pub fn main() {
    let result = extractMultipartBoundary("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
