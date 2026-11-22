# `cap:rss` — 零分配 RSS 2.0 Feed 解析、迭代与序列化标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

`cap:rss` 提供零堆分配的 RSS 2.0 Feed 识别、频道元数据解析、条目流式迭代以及规范化 XML 实体转义序列化能力，全面支持 `<![CDATA[...]]>` 剥离、标签属性、注释跳过与畸形 XML 防护。

## 核心 API

### 数据结构
- `RssChannel`：`title`, `link`, `description`, `language?`, `last_build_date?`
- `RssItem`：`title`, `link`, `description`, `guid?`, `pub_date?`, `author?`

### 函数与迭代器
- `isRss2Feed(xml: []const u8): bool` — 快速识别是否具备合法 `<rss ...><channel>...</channel></rss>` 结构（支持跳过 UTF-8 BOM、XML 声明与注释）
- `parseRssChannel(xml: []const u8): RssChannel?` — 提取 `<channel>` 级元数据，自动跳过 `<item>...</item>` 与 `<image>...</image>` 内部同名子标签，自动剥离 `<![CDATA[...]]>`
- `RssItemIter`：
  - `RssItemIter.init(xml: []const u8): RssItemIter`
  - `next(*_): RssItem?` — 零分配逐条迭代 `<item>...</item>`，支持任意属性（如 `<guid isPermaLink="false">`）与 CDATA
- `formatRssItem(item: RssItem, out: []u8): ([]const u8)?` — 对 `&`/`<`/`>`/`"`/`'` 进行标准 XML 转义并序列化为 `<item>...</item>`，缓冲不足返回 `null`

## 使用示例

```cp
import { isRss2Feed, parseRssChannel, RssItemIter, RssItem, formatRssItem } from "cap:doc";

fn demo(xml: []const u8) {
    if (!isRss2Feed(xml)) {
        return;
    }
    if (ch := parseRssChannel(xml)) {
        * = ch.title;
    }
    let it = RssItemIter.init(xml);
    while (item := it.next()) {
        let buf: [1024]u8 = undefined;
        if (encoded := formatRssItem(item, &buf)) {
            * = encoded;
        }
    }
}
```
