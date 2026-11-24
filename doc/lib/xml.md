# CP 标准库：XML 1.0 协议解析、路径查询与双向序列化手册 (`cap:xml`)

> 参考来源：`/data/dev/bun/src/s3_signing/`, `/data/dev/bun/src/test_runner/` | 吸收日期：2026-11-22

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

---

## 零分配 XML 1.0 文本/属性/CDATA 转义子模块（`escape.cp`，收敛自 `xml_escape`）

- `isValidXmlChar(cp: u32): bool`
- `escapeXmlText(src: []const u8, out: []u8): ([]const u8)?`
- `escapeXmlAttr(src: []const u8, out: []u8): ([]const u8)?`
- `wrapXmlCdata(src: []const u8, out: []u8): ([]const u8)?`
- `unescapeXml(src: []const u8, out: []u8): ([]const u8)?`

---

## 上游行为契约与安全防御注意事项

1. **XXE 与深层嵌套防护（`MAX_DEPTH = 128`）**：
   - `XmlParser` 仅支持 XML 1.0 内置 5 命名实体（`&amp;`、`&lt;`、`&gt;`、`&quot;`、`&apos;`）与十进制/十六进制数字字符引用，**天然免疫外部实体注入（XXE）与实体膨胀攻击**；超过 128 层嵌套标签直接报错中止。
2. **CDATA 终止序列安全分割（`wrapXmlCdata`）**：
   - XML 1.0 禁止 CDATA 段内直接出现 `]]>`；写入任意二进制或代码片段时使用 `wrapXmlCdata` 自动检测/拆分 `]]>` 序列，防止闭合注入破坏文档结构。
3. **合法 XML 1.0 字符过滤（`isValidXmlChar`）**：
   - XML 1.0 禁止 NUL 与多数 ASCII 控制字符（仅允许 `0x09`/`0x0A`/`0x0D` 及 `>= 0x20`），序列化日志或 JUnit 报告前可通过 `isValidXmlChar` 与 `escapeXmlText`/`escapeXmlAttr` 保证输出严格合规。

---

## Wave 35 工业级安全加固与 16-Lane SIMD 加速（对标 `RazrFalcon/roxmltree` MIT/Apache-2.0 & W3C XML 1.0 5th Ed）

`lib/xml/parser.cp`、`lib/xml/escape.cp` 与 `lib/xml/writer.cp` 已对照 `RazrFalcon/roxmltree`（MIT/Apache-2.0）与 W3C XML 1.0（5th Edition）完成严苛词法/文法加固、全栈零分配校验器与 16-lane `@Vector(16, u8)` SIMD 热路径加速，保持 100% 向后兼容：

| 加固项 / 新增符号 | 所在文件 | 工程价值 |
|---|---|---|
| `XML_MAX_DEPTH` (128) / `XML_MAX_ATTRS_PER_ELEMENT` (256) / `XML_MAX_NODES` (65536) / `XML_MAX_ENTITY_EXPANSIONS` (65536) | `lib/xml/parser.cp` | 强制限制元素嵌套深度、单元素属性数、总节点数与实体展开次数，防止深层嵌套与畸形大文档资源耗尽 |
| `XmlError` 细粒度异常（新增 `DuplicateAttribute`, `InvalidControlChar`, `MaxNodesExceeded`, `MaxAttrsExceeded`） | `lib/xml/parser.cp` | 严格拦截同元素重复属性、XML 1.0 §2.2 非法控制字节（`0x00..0x08`, `0x0B`, `0x0C`, `0x0E..0x1F`）、注释体 `--`、文本裸 `]]>`、非法数字引用（`0`/代理对/越界/大写 `&#X`）及非首部/非法 `<?xml ...?>` 声明 |
| `validateXmlStrict(xml_str)` | `lib/xml/parser.cp` | 零堆分配只读 W3C XML 1.0 严苛校验器（栈上定长深度与属性去重缓冲），与 `XmlParser.parse` 保持 100% 校验等价 |
| `simdFindXmlTextDelim` / `simdHasInvalidXmlControl` / `simdFindAttrQuoteOrAmp`（配标量参考） | `lib/xml/parser.cp` | 纯 CP 16-lane `@Vector(16, u8)` 文本定界符（`<`, `&`, `]`）、非法控制字符与属性引号/`&`/`<` 扫描，直接驱动解析器与校验器热路径 |

- **权威回归与 Fuzz 套件**：`cap/test/testXmlRoxmltreeRustRefSuite.cp` 覆盖 W3C XML 1.0 合规文档与序列化往返、10 类严苛异常拦截、全 256 字节值与 `0..257` 长度 16-lane SIMD vs 标量 0 差异对拍，以及 1,600 组确定性 PRNG 变异/随机字节流 Fuzz（同时校验 `validateXmlStrict` 与 `XmlParser.parse` 100% 一致性，0 panic / 0 OOB / 0 死循环）。
