# CP 标准库：XML 1.0 协议解析、路径查询与双向序列化手册 (`cap:xml`)

## 一、 架构总览与设计准则

`cap:xml` 是 CP 语言官方内置的纯 CP XML 1.0 协议解析与序列化标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，专为 S3 / RSS / Atom / SVG / JUnit XML 等数据交换场景设计。既可通过 `import { ... } from "cap:xml"` 导入，也支持 `import cap from "cap"; cap.xml.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层 XML 1.0 引擎 (cap:xml)                   │
├──────────────────────────────┬──────────────────────────────────────────┤
│ DOM 解析与路径查询 (parser.cp) │ 紧凑与缩进序列化 (writer.cp)               │
│ XmlParser / XmlDocument      │ XmlWriter (流式构建 + 属性/文本自动转义)     │
│ XmlNode / XmlAttr / Kind     │ XmlSerializer.stringify (DOM 双向无损还原) │
│ 实体解码 / MAX_DEPTH=128 防护 │ CDATA `]]>` 注入校验与安全关闭             │
└──────────────────────────────┴──────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/xml/parser.cp` | `// Source: cp/lib/xml/parser.cp (XML 1.0 DOM Parser, Entity Decoder & Path Query)` | `XmlError`、`XmlAttr`、`XmlNodeKind`、`XmlNode`、`XmlDocument`、`XmlParser` |
| `lib/xml/writer.cp` | `// Source: cp/lib/xml/writer.cp (XML 1.0 Compact & Pretty Serializer)` | `XmlWriter`（流式紧凑/美化写入器）与 `XmlSerializer` |
| `lib/xml/index.cp` | `// Source: cp/lib/xml/index.cp (Unified Exports for cap:xml)` | `cap:xml` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与 DOM 模型（`parser.cp`）

- **`XmlError`**：`UnexpectedEof`、`MismatchedTag`、`MalformedTag`、`MaxDepthExceeded`、`InvalidEntity`、`OutOfMemory`。
- **`XmlAttr`**：`name: []const u8`、`value: []const u8`（解析时自动完成实体解码）。
- **`XmlNodeKind`**：`Element`、`Text`、`CData`。
- **`XmlDocument`**：
  - `version: []const u8`、`encoding: []const u8`、`root: XmlNode`；
  - 内部由独立 `std.heap.ArenaAllocator` 承载整棵 DOM 树节点、属性及解码字符串，调用 `doc.deinit()` 即可 $O(1)$ 一键释放全部解析内存。
- **`XmlNode` 便携 DOM 查询**：
  - `getAttr(name): ([]const u8)?`：按属性名精确查询解码后的属性值；
  - `findChild(tag): XmlNode?`：查找首个匹配的直接子元素；
  - `findAllChildren(alloc, tag): ([]XmlNode)!`：收集全部匹配的直接子元素切片（调用方负责 `alloc.free(...)`）；
  - `findByPath(path): XmlNode?`：按 `/` 分隔相对路径逐级查找元素（如 `"channel/item/title"`）；
  - `innerText(alloc): ([]u8)!`：按文档顺序递归拼接所有 `Text` 与 `CData` 子节点文本（调用方负责 `alloc.free(...)`）。

### 2. XML 1.0 解析器特性（`XmlParser.parse(alloc, xml_str)`）

- 支持 UTF-8 BOM 自动剥离、`<?xml version="..." encoding="..."?>` 声明及任意 `<?pi ...?>` 处理指令；
- 支持 `<!-- ... -->` 注释跳过、`<!DOCTYPE ...>` 前导跳过、`<![CDATA[...]]>` 原样文本节点；
- 支持自闭合标签 `<item/>`、带命名空间前缀标签 `ns:tag` 与属性 `xmlns:dc="..."`、单引号 `'...'` 与双引号 `"..."` 属性；
- 严格实体解码：支持 XML 5 大预定义实体（`&amp;`、`&lt;`、`&gt;`、`&quot;`、`&apos;`）以及十进制 `&#NNN;`、十六进制 `&#xHHH;` 数字字符引用（校验合法 XML 1.0 Unicode 码点，拒绝未知实体或孤立 `&`）；
- 异常边界防护：自动拒绝不匹配闭合标签（`MismatchedTag`）、未闭合标签/声明/CDATA（`UnexpectedEof`）及超过 `MAX_DEPTH = 128` 层的深度嵌套（`MaxDepthExceeded`）。

### 3. 流式写入器与双向序列化（`writer.cp`）

- **`XmlWriter`**：
  - `init(alloc)`：紧凑模式初始化；
  - `initPretty(alloc, indent)`：按 `indent` 空格缩进的美化模式初始化；
  - `writeDeclaration()`：写入 `<?xml version="1.0" encoding="UTF-8"?>`；
  - `openTag(name)` / `attr(name, value)` / `text(content)` / `cdata(content)` / `emptyTag(name)` / `closeTag()`：
    - `attr` 自动转义 `&`、`<`、`>`、`"`、`'` 及控制换行符；
    - `text` 自动转义 `&`、`<`、`>`；
    - `cdata` 严格校验并拒绝包含 `]]>` 的非法载荷（返回 `err.MalformedTag`）；
  - `toOwnedSlice()` / `deinit()`。
- **`XmlSerializer.stringify(alloc, node, pretty): ([]u8)!`**：将任意 `XmlNode` DOM 子树无损序列化为紧凑或缩进美化 XML 文本。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { XmlParser, XmlWriter, XmlSerializer } from "cap:xml";
import cap from "cap";

pub fn demoXmlPipeline(): ! {
    const alloc = std.heap.page_allocator;
    const rss =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<rss version=\"2.0\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n" ++
        "  <channel>\n" ++
        "    <title>CP &amp; Zig Feed</title>\n" ++
        "    <item><title><![CDATA[Hello <XML>]]></title><dc:creator>Alice</dc:creator></item>\n" ++
        "  </channel>\n" ++
        "</rss>";

    let doc = XmlParser.parse(alloc, rss)?;
    defer doc.deinit();

    const title_node = doc.root.findByPath("channel/item/title").?;
    const title_text = title_node.innerText(alloc)?;
    defer alloc.free(title_text);

    const pretty_xml = XmlSerializer.stringify(alloc, doc.root, true)?;
    defer alloc.free(pretty_xml);

    // 动态命名空间 cap.xml
    let w = cap.xml.XmlWriter.init(alloc);
    defer w.deinit();
    w.writeDeclaration()?;
    w.openTag("ping")?;
    w.attr("ok", "true")?;
    w.closeTag()?;
    const out = w.toOwnedSlice()?;
    defer alloc.free(out);
    * = title_text;
}
```
