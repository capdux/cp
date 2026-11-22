# `cap:docx` — 纯 CP OOXML DOCX WordprocessingML 与核心元数据零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

## 概述

`cap:docx` 提供纯 CP、零外部依赖、零堆分配的 OOXML DOCX（`word/document.xml`、`docProps/core.xml`、`[Content_Types].xml`）解析与纯文本提取能力：

- **格式判定**：精准识别 WordprocessingML `<w:document>` / `<document>` 根元素（自动跳过 UTF-8 BOM、XML 声明与注释）与 `[Content_Types].xml` 主文档 MIME 声明；
- **核心元数据解析**：零拷贝解析 `docProps/core.xml` 全字段（兼容 `dc:` / `cp:` / `dcterms:` 命名空间及无前缀形式）；
- **段落零分配迭代**：精准区分 `<w:p>` 与 `<w:pPr>` / `<w:pStyle>` / `<w:pBdr>` 等同前缀标签，支持嵌套 `<w:p>` 深度匹配与 `<w:pStyle w:val="..."/>` 标题识别；
- **纯文本零堆提取**：提取 `<w:t>` / `<w:t xml:space="preserve">` 文本节点，支持完整 XML 命名与数字实体反转义（`&amp;` / `&lt;` / `&gt;` / `&quot;` / `&apos;` / `&#dec;` / `&#xhex;`）、`<w:tab/>` -> `'\t'`、`<w:br/>` / `<w:cr/>` -> `'\n'`、段落间 `'\n'` 分隔，缓冲区溢出安全返回 `null`；
- **文档统计汇总**：统计段落数、标题数、表格数（`<w:tbl>`）与文本运行块数（`<w:r>`）。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `DOCX_MAIN_CONTENT_TYPE` | OOXML WordprocessingML 主文档 Content-Type 常量 |
| `isWordDocumentXml(xml): bool` | 判定 XML 是否以 `<w:document` 或 `<document` 为根标签 |
| `isDocxContentTypes(xml): bool` | 判定 `[Content_Types].xml` 是否包含 `DOCX_MAIN_CONTENT_TYPE` |
| `DocxCoreProps` | `docProps/core.xml` 零拷贝元数据：`title`, `subject`, `creator`, `keywords`, `description`, `last_modified_by`, `revision`, `created`, `modified` |
| `parseDocxCoreProps(core_xml): DocxCoreProps?` | 解析 `docProps/core.xml` 全字段（非 `coreProperties` 返回 `null`） |
| `DocxParagraph` | 段落视图：`raw_xml: []const u8`, `style: []const u8`, `is_heading: bool` |
| `DocxParagraphIter` | 零分配段落迭代器：`init(doc_xml)`、`next(*_): DocxParagraph?` |
| `extractDocxParagraphText(para_xml, out): usize?` | 提取单个 `<w:p>` 段落纯文本至调用方缓冲区 `out`，溢出返回 `null` |
| `extractDocxText(doc_xml, out): usize?` | 提取整份 `word/document.xml` 纯文本（段落间 `'\n'` 分隔），非 DOCX 或溢出返回 `null` |
| `DocxStats` | 文档统计：`paragraph_count`, `heading_count`, `table_count`, `run_count` |
| `summarizeDocx(doc_xml): DocxStats` | 统计 `word/document.xml` 段落、标题、表格与运行块数量 |

## 示例

```cp
import std from "std";
import {
    isWordDocumentXml,
    parseDocxCoreProps,
    DocxParagraphIter,
    extractDocxText,
    summarizeDocx,
} from "cap:doc";

fn readDocx(doc_xml: []const u8, core_xml: []const u8): usize {
    if (!isWordDocumentXml(doc_xml)) return 0;

    if (props := parseDocxCoreProps(core_xml)) {
        * = props.title;
        * = props.creator;
    }

    const stats = summarizeDocx(doc_xml);
    * = stats.paragraph_count;

    let buf: [4096]u8 = std.mem.zeroes([4096]u8);
    return extractDocxText(doc_xml, buf[0..]) ?? 0;
}
```
