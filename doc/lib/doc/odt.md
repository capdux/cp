# `cap:odt` — OASIS OpenDocument (`.odt`/`.ods`/`.odp`) 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

> **导入方式**：`import { detectOdfMimetype, isOdtZip, parseOdtMeta, OdtManifestIter, OdtBlockIter, extractOdtText } from "cap:doc";`  
> **内存模型**：100% 纯 CP 零堆分配，元数据、清单与块切片零拷贝指向原始输入，纯文本提取直接写入调用方缓冲区。

---

## 核心常量

| 常量 | 值 |
| :--- | :--- |
| `ODT_MIMETYPE` | `"application/vnd.oasis.opendocument.text"` |
| `ODS_MIMETYPE` | `"application/vnd.oasis.opendocument.spreadsheet"` |
| `ODP_MIMETYPE` | `"application/vnd.oasis.opendocument.presentation"` |

---

## 核心数据结构

- **`OdtMeta`**：`title`, `description`, `subject`, `creator`, `initial_creator`, `creation_date`, `date`, `language`, `generator`（均 `[]const u8`），以及 `page_count: u32`, `word_count: u32`, `character_count: u32`。
- **`OdtManifestEntry`**：`full_path: []const u8`, `media_type: []const u8`, `version: []const u8`。
- **`OdtBlock`**：`is_heading: bool`, `outline_level: u8`, `style_name: []const u8`, `raw_xml: []const u8`。

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `detectOdfMimetype` | `fn detectOdfMimetype(zip_data: []const u8): ([]const u8)?` | 按 OASIS OCF 规范校验 ZIP 首文件为未压缩（method=0）、无 extra、文件名为 `"mimetype"` 并返回其 MIME 切片 |
| `isOdtZip` | `fn isOdtZip(zip_data: []const u8): bool` | 判断 ZIP 是否为合法 `.odt`（`ODT_MIMETYPE`） |
| `parseOdtMeta` | `fn parseOdtMeta(meta_xml: []const u8): OdtMeta?` | 零分配解析 `meta.xml` 中的 `dc:*`、`meta:*` 与 `<meta:document-statistic>` |
| `OdtManifestIter` | `OdtManifestIter.init(manifest_xml)` / `it.next(): OdtManifestEntry?` | 零分配迭代 `META-INF/manifest.xml` 中全部 `<manifest:file-entry>` |
| `OdtBlockIter` | `OdtBlockIter.init(content_xml)` / `it.next(): OdtBlock?` | 零分配迭代 `content.xml` 中全部 `<text:h>` 与 `<text:p>` |
| `extractOdtText` | `fn extractOdtText(content_xml: []const u8, out: []u8): usize?` | 零堆分配提取纯文本，支持 `<text:s text:c="N"/>`、`<text:tab/>`、`<text:line-break/>`、嵌套 `<text:span>` 与 XML 实体反转义，溢出返回 `null` |

---

## 使用示例

```typescript
import { isOdtZip, parseOdtMeta, extractOdtText } from "cap:doc";

fn inspectOdt(zip_bytes: []const u8, meta_xml: []const u8, content_xml: []const u8) {
    if (!isOdtZip(zip_bytes)) {
        return;
    }
    if (meta := parseOdtMeta(meta_xml)) {
        * = meta.title;
        * = meta.word_count;
    }
    let out_buf: [4096]u8 = undefined;
    if (n := extractOdtText(content_xml, out_buf[0..])) {
        const plain = out_buf[0..n];
        * = plain;
    }
}
```
