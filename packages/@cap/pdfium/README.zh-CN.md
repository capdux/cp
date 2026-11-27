[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/pdfium

Google PDFium PDF rendering & text extraction engine (prebuilt dynamic library) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/pdfium` |
| 版本 | `131.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/pdfium
```

```cp
import {
    PdfError,
    PageSize,
    PdfDoc,
    FPDF_InitLibrary,
    FPDF_DestroyLibrary,
    FPDF_LoadMemDocument,
    FPDF_CloseDocument,
    FPDF_GetPageCount,
} from "@cap/pdfium";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `PdfError` | `OpenFailed, InvalidPage, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PageSize` | `class` | 字段: `width: f64, height: f64` |
| `PdfDoc` | `class` | 字段: `doc_ptr: *auto!?` · 方法: `loadFromMemory, pageCount, pageSize, pageCharCount, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `FPDF_InitLibrary` | `()` | 就地执行 `FPDF_InitLibrary` 操作。 |
| `FPDF_DestroyLibrary` | `()` | 就地执行 `FPDF_DestroyLibrary` 操作。 |
| `FPDF_LoadMemDocument` | `(data_buf: *auto!?, size: i32, password: [*:0]&u8!?): *auto!?` | 调用 `FPDF_LoadMemDocument` 并返回 `*auto!?`。 |
| `FPDF_CloseDocument` | `(document: *auto!?)` | 就地执行 `FPDF_CloseDocument` 操作。 |
| `FPDF_GetPageCount` | `(document: *auto!?): i32` | 调用 `FPDF_GetPageCount` 并返回 `i32`。 |
| `FPDF_LoadPage` | `(document: *auto!?, page_index: i32): *auto!?` | 调用 `FPDF_LoadPage` 并返回 `*auto!?`。 |
| `FPDF_ClosePage` | `(page: *auto!?)` | 就地执行 `FPDF_ClosePage` 操作。 |
| `FPDF_GetPageWidth` | `(page: *auto!?): f64` | 调用 `FPDF_GetPageWidth` 并返回 `f64`。 |
| `FPDF_GetPageHeight` | `(page: *auto!?): f64` | 调用 `FPDF_GetPageHeight` 并返回 `f64`。 |
| `FPDFText_LoadPage` | `(page: *auto!?): *auto!?` | 调用 `FPDFText_LoadPage` 并返回 `*auto!?`。 |
| `FPDFText_ClosePage` | `(text_page: *auto!?)` | 就地执行 `FPDFText_ClosePage` 操作。 |
| `FPDFText_CountChars` | `(text_page: *auto!?): i32` | 调用 `FPDFText_CountChars` 并返回 `i32`。 |
| `FPDFText_GetText` | `(text_page: *auto!?, start_index: i32, count: i32, result: [*]u16!): i32` | 调用 `FPDFText_GetText` 并返回 `i32`。 |
| `initLibrary` | `()` | 创建或初始化实例，返回 `void`。 |
| `destroyLibrary` | `()` | 释放模块分配的底层资源。 |

## 最小调用示例

```cp
import { FPDF_InitLibrary } from "@cap/pdfium";

pub fn main() {
    FPDF_InitLibrary();
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
