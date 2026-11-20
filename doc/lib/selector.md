# CP 标准库：纯 CP CSS3 选择器编译器与 DOM 树查询引擎 (`cap:selector`)

## 一、 架构总览与设计准则

`cap:selector` 是 CP 语言官方内置的工业级纯 CP **CSS3 选择器编译器与 XML/DOM 树查询引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，直接无缝对接 `cap:xml` (`XmlNode` / `XmlDocument`)，同时支持 `import { ... } from "cap:selector"` 与 `import cap from "cap"; cap.selector.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP 网页抽取与 AI Agent DOM 查询引擎 (cap:selector)      │
├──────────────────────┬──────────────────────────┬───────────────────────┤
│ CSS3 选择器预编译器   │ 从右向左反向匹配器        │ 先序文档序去重查询     │
│ CssSelector.compile  │ matches(node, root)      │ queryAll / queryFirst │
│ SelectorError        │ 4 种组合器 + 6 种伪类     │ selectXml / First     │
└──────────────────────┴──────────────────────────┴───────────────────────┘
```

### 1. 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/selector/selector.cp` | `// Source: cp/lib/selector/selector.cp (...)` | `SelectorError`、`AttrOp`、`Combinator`、`CssSelector` AST 编译器、反向匹配引擎与 `cap:xml` 一步查询 |
| `lib/selector/index.cp` | `// Source: cp/lib/selector/index.cp (...)` | `cap:selector` 统一公开导出入口 |

---

## 二、 支持的 CSS3 语法矩阵

- **基础选择器**：标签（`div`、`svg:rect`）、ID（`#main`）、多类交集（`.btn.primary`）、通配符（`*`）
- **属性选择器**：`[attr]`（存在）、`[attr=v]`（相等）、`[attr^=v]`（前缀）、`[attr$=v]`（后缀）、`[attr*=v]`（子串）、`[attr~=v]`（空白分隔词），支持单引号、双引号、无引号及可选 `i`/`s` 大小写修饰符
- **组合器**：后代（`A B`）、直接子代（`A > B`）、相邻兄弟（`A + B`）、通用兄弟（`A ~ B`）
- **结构与否定伪类**：`:first-child`、`:last-child`、`:only-child`、`:empty`、`:nth-child(odd | even | an+b)`、`:not(compound)`
- **选择器列表**：逗号并集（`h1, h2, .card`），按先序遍历严格去重并保持文档顺序返回

---

## 三、 核心 API 详解

### 1. 错误与枚举类型

- **`SelectorError`**：`EmptySelector`、`UnexpectedToken`、`UnclosedBracket`、`UnclosedParen`、`InvalidNthExpr`、`UnsupportedPseudo`、`OutOfMemory`
- **`AttrOp`**：`Exists`、`Equals`、`Prefix`、`Suffix`、`Substring`、`Word`
- **`Combinator`**：`Descendant`、`Child`、`AdjacentSibling`、`GeneralSibling`
- **`NodePtr`**：`&XmlNode`（指向 `cap:xml` DOM 节点）

### 2. `CssSelector` 方法

- `CssSelector.compile(alloc: std.mem.Allocator, expr: []const u8): CssSelector ! SelectorError`
  预编译 CSS3 选择器列表为 Arena 托管的结构化 AST；严格拦截空选择器、未闭合 `[` / `(`、非法 `:nth-child` 表达式与未知伪类。
- `deinit(*_)`
  O(1) 释放预编译 AST 占用的 Arena 内存。
- `matches(node: NodePtr, root: NodePtr): bool`
  从右向左反向匹配复杂选择器链，判断 `root` 树中的 `node` 是否命中。
- `queryAll(alloc: std.mem.Allocator, root: NodePtr): ([]NodePtr) ! SelectorError`
  先序遍历 `root` DOM 树，返回所有命中节点（多选择器 `,` 自动去重并保持文档序）；调用方使用 `alloc.free(...)` 释放结果切片。
- `queryFirst(root: NodePtr): NodePtr?`
  返回文档序中第一个命中的 `NodePtr`，未命中返回 `null`。
- `CssSelector.selectXml(alloc, xml_text, css_expr): ([][]u8)!` / `CssSelector.freeTexts(alloc, texts)`
  一步完成 `cap:xml` 解析 + CSS3 查询并提取所有命中节点的 `innerText`。
- `CssSelector.selectXmlFirst(alloc, xml_text, css_expr): ([]u8)?!`
  一步完成 `cap:xml` 解析 + CSS3 查询并返回首个命中节点的 `innerText`。

---

## 四、 可运行代码示例

```typescript
const std = @import("std");
import { XmlParser } from "cap:xml";
import { CssSelector } from "cap:selector";
import cap from "cap";

pub fn demoSelector(): ! {
    const alloc = std.heap.page_allocator;
    const html =
        "<section id=\"main\">" ++
        "  <h2>News</h2>" ++
        "  <p class=\"lead\">First</p>" ++
        "  <p class=\"lead disabled\">Second</p>" ++
        "  <a class=\"btn primary\" href=\"https://cp-lang.org/spec.pdf\">Spec</a>" ++
        "</section>";

    let doc = XmlParser.parse(alloc, html)?;
    defer doc.deinit();

    let sel = CssSelector.compile(alloc, "h2 + p:not(.disabled), a.btn.primary[href$=\".pdf\"]")?;
    defer sel.deinit();

    const hits = sel.queryAll(alloc, &doc.root)?;
    defer alloc.free(hits);

    // 一步查询 XML 并提取文本（含 cap.selector 动态挂载）
    const texts = cap.selector.CssSelector.selectXml(alloc, html, "section#main > p.lead")?;
    defer cap.selector.CssSelector.freeTexts(alloc, texts);
}
```
