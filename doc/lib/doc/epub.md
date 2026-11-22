# `cap:epub` — EPUB 2/3 OCF & OPF 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

> **导入方式**：`import { isEpubZip, parseContainerXml, parseOpfMetadata, EpubManifestIter, findManifestItem, EpubSpineIter } from "cap:doc";`  
> **内存模型**：100% 纯 CP 零堆分配，所有返回切片直接零拷贝指向原始输入缓冲区。

---

## 核心常量

| 常量 | 值 | 说明 |
| :--- | :--- | :--- |
| `EPUB_MIMETYPE` | `"application/epub+zip"` | EPUB OCF 规范规定的 `mimetype` 文件内容 |
| `EPUB_OPF_MEDIA_TYPE` | `"application/oebps-package+xml"` | OPF Package 文档的标准 `media-type` |

---

## 核心数据结构

### `EpubRootfile`
`META-INF/container.xml` 中 `<rootfile>` 的解析结果：
- `full_path: []const u8` — OPF 文件在容器内的路径（如 `"OEBPS/content.opf"`）
- `media_type: []const u8` — 媒体类型（通常为 `EPUB_OPF_MEDIA_TYPE`）

### `EpubMetadata`
OPF `<package>` 属性与 `<metadata>` 区块字段（缺失字段为空切片 `""`）：
- `version: []const u8` — OPF 版本（如 `"2.0"`、`"3.0"`）
- `unique_identifier: []const u8` — `<package unique-identifier="...">` 指定的 ID 引用
- `title: []const u8` — 书名（`<dc:title>` / `<title>`）
- `creator: []const u8` — 作者（`<dc:creator>` / `<creator>`）
- `language: []const u8` — 语言（`<dc:language>` / `<language>`）
- `identifier: []const u8` — 唯一标识符（优先匹配 `id == unique_identifier` 的 `<dc:identifier>`）
- `publisher: []const u8` — 出版商（`<dc:publisher>` / `<publisher>`）
- `date: []const u8` — 日期（`<dc:date>` / `<date>`）
- `description: []const u8` — 简介（`<dc:description>` / `<description>`）

### `EpubManifestItem`
OPF `<manifest>` 中单个 `<item>` 资源条目：
- `id: []const u8`
- `href: []const u8`
- `media_type: []const u8`
- `properties: []const u8`（可选，缺省为空切片 `""`，如 `"nav"`、`"cover-image"`）

### `EpubSpineItem`
OPF `<spine>` 中单个 `<itemref>` 阅读顺序条目：
- `idref: []const u8` — 对应 `<manifest>` 中 `<item id="...">`
- `linear: bool` — 默认 `true`；当 `linear="no"`（忽略大小写）时为 `false`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isEpubZip` | `fn isEpubZip(data: []const u8): bool` | 按 EPUB OCF 规范严格校验 ZIP 首个 Local File Header（`PK\x03\x04`、method=0、extra_len=0、offset 30..58 严格匹配 `"mimetypeapplication/epub+zip"`） |
| `parseContainerXml` | `fn parseContainerXml(xml: []const u8): EpubRootfile?` | 零分配提取 `META-INF/container.xml` 中的 `<rootfile>`（优先匹配 `EPUB_OPF_MEDIA_TYPE`，兼容单双引号与任意属性顺序） |
| `parseOpfMetadata` | `fn parseOpfMetadata(opf_xml: []const u8): EpubMetadata?` | 零分配解析 OPF `<package>` 与 `<metadata>`（兼容 `dc:` 命名空间前缀、无前缀、CDATA 及带属性标签） |
| `EpubManifestIter` | `EpubManifestIter.init(opf_xml)` / `it.next(): EpubManifestItem?` | 零分配顺序遍历 `<manifest>` 中的全部 `<item>` |
| `findManifestItem` | `fn findManifestItem(opf_xml: []const u8, id: []const u8): EpubManifestItem?` | 按 `id` 零分配查找 `<manifest>` 中的 `<item>` |
| `EpubSpineIter` | `EpubSpineIter.init(opf_xml)` / `it.next(): EpubSpineItem?` | 零分配顺序遍历 `<spine>` 中的全部 `<itemref>` |

---

## 使用示例

```typescript
import {
    isEpubZip,
    parseContainerXml,
    parseOpfMetadata,
    EpubManifestIter,
    findManifestItem,
    EpubSpineIter,
} from "cap:doc";

fn inspectEpub(zip_bytes: []const u8, container_xml: []const u8, opf_xml: []const u8) {
    if (!isEpubZip(zip_bytes)) {
        return;
    }
    if (rf := parseContainerXml(container_xml)) {
        // rf.full_path => "OEBPS/content.opf"
        * = rf.full_path;
    }
    if (meta := parseOpfMetadata(opf_xml)) {
        // meta.title, meta.creator, meta.language, meta.identifier
        * = meta.title;
    }
    let spine = EpubSpineIter.init(opf_xml);
    while (entry := spine.next()) {
        if (entry.linear) {
            if (item := findManifestItem(opf_xml, entry.idref)) {
                * = item.href;
            }
        }
    }
}
```
