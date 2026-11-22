# `cap:opml` — 零分配 OPML 2.0 大纲/订阅列表解析与序列化标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

`cap:opml` 提供零堆分配的 OPML 2.0 文档识别、`<head>` 元数据解析、嵌套 `<outline>` 层级流式迭代以及规范化 XML 属性转义序列化能力，与 `cap:rss` / `cap:atom` 组成完整的 Feed 聚合三件套。

## 核心 API

### 数据结构
- `OpmlHead`：`title?`, `date_created?`, `owner_name?`
- `OpmlOutline`：`text`, `title?`, `kind?`（对应 `type` 属性）, `xml_url?`, `html_url?`, `depth: u16`
  - `isFeed(): bool` — 当 `xml_url != null` 时返回 `true`

### 函数与迭代器
- `isOpmlDocument(xml: []const u8): bool` — 跳过 UTF-8 BOM、XML 声明与注释，校验 `<opml ...>` 与 `<body>` 结构
- `parseOpmlHead(xml: []const u8): OpmlHead?` — 提取 `<head>` 内的 `<title>`、`<dateCreated>`、`<ownerName>`
- `OpmlOutlineIter`：
  - `OpmlOutlineIter.init(xml: []const u8): OpmlOutlineIter`
  - `next(*_): OpmlOutline?` — 零分配逐条迭代 `<outline ...>`，精确追踪嵌套与自闭合标签的 `depth`，支持单/双引号属性
- `formatOpmlFeedOutline(text, xml_url, html_url?, out): ([]const u8)?` — 对属性执行标准 XML 转义并输出 `<outline type="rss" .../>`，缓冲不足返回 `null`

## 使用示例

```cp
import { isOpmlDocument, parseOpmlHead, OpmlOutlineIter, formatOpmlFeedOutline } from "cap:web";

fn demo(xml: []const u8) {
    if (!isOpmlDocument(xml)) {
        return;
    }
    if (head := parseOpmlHead(xml)) {
        * = head.title;
    }
    let it = OpmlOutlineIter.init(xml);
    while (node := it.next()) {
        if (node.isFeed()) {
            let buf: [512]u8 = undefined;
            if (line := formatOpmlFeedOutline(node.text, node.xml_url.?, node.html_url, &buf)) {
                * = line;
            }
        }
    }
}
```
