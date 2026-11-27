[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/pdfium

> 版本：`131.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Google PDFium PDF rendering & text extraction engine (prebuilt dynamic library) for CP

## 安装与导入

```bash
cap add @cap/pdfium
```

```cp
import { FPDF_InitLibrary } from "@cap/pdfium";
```

## 核心导出 API

- `export fn FPDF_InitLibrary();`
- `export fn FPDF_DestroyLibrary();`
- `export fn FPDF_LoadMemDocument(data_buf: *auto!?, size: i32, password: [*:0]&u8!?): *auto!?;`
- `export fn FPDF_CloseDocument(document: *auto!?);`
- `export fn FPDF_GetPageCount(document: *auto!?): i32;`
- `export fn FPDF_LoadPage(document: *auto!?, page_index: i32): *auto!?;`
- `export fn FPDF_ClosePage(page: *auto!?);`
- `export fn FPDF_GetPageWidth(page: *auto!?): f64;`
- `export fn FPDF_GetPageHeight(page: *auto!?): f64;`
- `export fn FPDFText_LoadPage(page: *auto!?): *auto!?;`
- `export fn FPDFText_ClosePage(text_page: *auto!?);`
- `export fn FPDFText_CountChars(text_page: *auto!?): i32;`
- `export fn FPDFText_GetText(text_page: *auto!?, start_index: i32, count: i32, result: [*]u16!): i32;`
- `export fn initLibrary();`
- `export fn destroyLibrary();`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`pdfium@131.0.0`)，许可证：BSD-3-Clause
