# CP 标准库：PDF 1.4 文档生成与内省引擎手册 (`cap:pdf`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

## 一、 架构总览与设计准则

`cap:pdf` 是 CP 语言官方内置的纯 CP PDF 1.4 多页文档生成与结构内省标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既可通过 `import { ... } from "cap:doc"` 导入，也支持 `import cap from "cap"; cap.pdf.*` 自动动态懒挂载调用。

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                     CP 用户层 PDF 1.4 引擎 (cap:pdf)                        │
├─────────────────────────────────────┬──────────────────────────────────────┤
│ 文档生成与矢量排版 (writer.cp)        │ 结构校验与元数据内省 (inspector.cp)    │
│ PdfFont (4 大 Base14 标准字体)       │ PdfInspector.extractVersion          │
│ PageSize (A4 / Letter / Custom)     │ PdfInspector.isValidPdf              │
│ PdfPage (文本转义/RGB/线段/矩形)      │ PdfInspector.inspect -> PdfInfo      │
│ PdfDocument (多页/Info/10位xref表)   │ 校验 xref/startxref/%%EOF 与 /Page数 │
└─────────────────────────────────────┴──────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/pdf/writer.cp` | `// Source: cp/lib/pdf/writer.cp (Zero-Dependency PDF 1.4 Multi-Page Document Writer)` | `PdfError`、`PdfFont`、`PageSize`、`PdfPage`、`PdfDocument` |
| `lib/pdf/inspector.cp` | `// Source: cp/lib/pdf/inspector.cp (PDF Header, XRef, Page Count & Info Metadata Inspector)` | `PdfInfo`、`PdfInspector` |
| `lib/pdf/index.cp` | `// Source: cp/lib/pdf/index.cp (Unified Exports for cap:pdf)` | `cap:pdf` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合、字体与页面尺寸（`writer.cp`）

- **`PdfError`**：`InvalidPdf`、`UnsupportedVersion`、`MissingXRef`、`MissingEof`、`InvalidArgument`、`OutOfMemory`。
- **`PdfFont`**（Base14 内置 Type1 标准字体，无需嵌字）：
  - `Helvetica`（`/F1` -> `/Helvetica`）
  - `HelveticaBold`（`/F2` -> `/Helvetica-Bold`）
  - `Courier`（`/F3` -> `/Courier`）
  - `TimesRoman`（`/F4` -> `/Times-Roman`）
- **`PageSize`**（单位为 PDF 点，`1 pt = 1/72 inch`）：
  - `PageSize.a4()`：`595.28 x 841.89`
  - `PageSize.letter()`：`612 x 792`
  - `PageSize.custom(width, height)`：自定义页面宽高

### 2. 页面画布绘制（`PdfPage`）

- `drawText(x, y, font, size, text)`：在 `(x, y)` 以指定 Base14 字体与字号绘制文本，自动对 PDF 字面量中的 `\`、`(`、`)`、`\r`、`\n`、`\t` 进行严格转义；
- `setFillRgb(r, g, b)`：设置非描边填充 RGB 颜色（`0.0 .. 1.0`）；
- `setStrokeRgb(r, g, b)`：设置描边 RGB 颜色（`0.0 .. 1.0`）；
- `setLineWidth(w)`：设置描边线宽（`w >= 0`）；
- `drawLine(x1, y1, x2, y2)`：绘制并描边直线段；
- `drawRect(x, y, w, h)`：绘制并描边矩形；
- `fillRect(x, y, w, h)`：填充实心矩形；
- `contentSlice()`：获取当前页面原始 PDF 内容流只读切片。

### 3. 多页文档构建器（`PdfDocument`）

- `PdfDocument.init(alloc)` / `deinit()`：初始化并托管所有页面与元数据生命周期；
- `setTitle(t)` / `setAuthor(a)` / `setCreator(c)`：设置 `/Info` 字典元数据；
- `addPage(size): (*PdfPage) ! PdfError`：新增指定尺寸页面并返回稳定可变页面指针；
- `pageCount(): usize`：返回当前页数；
- `build(alloc): ([]u8) ! PdfError`：生成标准 `%PDF-1.4\n` 字节流，包含 `Catalog`、`Pages`、`Font`、逐页 `Page` 与 `Content Stream`、可选 `Info`、精确 20 字节条目（10 位零填充字节偏移）`xref` 表、`trailer`、`startxref` 与 `%%EOF`（调用方负责 `alloc.free(...)`）。

### 4. 结构与元数据内省（`inspector.cp`）

- **`PdfInfo`**：`version: []const u8`、`page_count: usize`、`has_xref: bool`、`has_eof: bool`、`title: ([]const u8)?`、`author: ([]const u8)?`、`creator: ([]const u8)?`。
- **`PdfInspector`**：
  - `extractVersion(bytes): string?`：校验并提取 `%PDF-X.Y` 版本号（如 `"1.4"`）；
  - `isValidPdf(bytes): bool`：联合校验头部版本、`startxref` 指向的 `xref` 表与末尾 `%%EOF`；
  - `inspect(bytes): PdfInfo ! PdfError`：严格验证文档结构后，跳过 `stream ... endstream` 负载精准统计 `/Type /Page` 页数（排除 `/Type /Pages`），并提取 `/Title`、`/Author`、`/Creator` 元数据。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { PdfDocument, PdfFont, PageSize, PdfInspector } from "cap:doc";
import cap from "cap";

pub fn demoPdfPipeline(): ! {
    const alloc = std.heap.page_allocator;

    let doc = PdfDocument.init(alloc);
    defer doc.deinit();

    doc.setTitle("CP Architecture Report")?;
    doc.setAuthor("CP Team")?;
    doc.setCreator("cap:doc")?;

    const p1 = doc.addPage(PageSize.a4())?;
    p1.setFillRgb(0.1, 0.2, 0.5)?;
    p1.fillRect(48.0, 760.0, 500.0, 40.0)?;
    p1.setFillRgb(1.0, 1.0, 1.0)?;
    p1.drawText(60.0, 774.0, PdfFont.HelveticaBold, 16.0, "Hello (PDF 1.4) \\ CP!")?;

    const bytes = doc.build(alloc)?;
    defer alloc.free(bytes);

    const info = PdfInspector.inspect(bytes)?;
    const valid = cap.pdf.PdfInspector.isValidPdf(bytes);
    * = info;
    * = valid;
}
```
