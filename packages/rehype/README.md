[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/rehype-ast

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rehype` ([https://github.com/rehypejs/rehype](https://github.com/rehypejs/rehype))（原库名 `rehype` → CP 包名 `@cap/rehype-ast`）

Zero-allocation HAST (Hypertext Abstract Syntax Tree) builder & HTML5 serializer for CP — clean-room MIT parity with npm `rehype` / `hast`. Composes `@cap/escape-html` with zero duplication.

纯 CP 零堆分配 HTML AST 构建与 HTML5 序列化库（对标 npm `rehype` / `hast`），复用 `@cap/escape-html` 自动转义文本与属性值防止 XSS，支持全部 14 个 HTML5 void 元素与纯文本提取。

## Install / 安装

```bash
cap add @cap/rehype-ast
```

## Dependencies / 跨包复用

- `@cap/escape-html`: 单遍零堆分配 HTML 实体转义（`<`, `>`, `&`, `"`, `'`）

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `HAST_*` | `HAST_ROOT (0)` / `HAST_ELEMENT (1)` / `HAST_TEXT (2)` / `HAST_COMMENT (3)` / `HAST_DOCTYPE (4)` | HAST node kind constants |
| `isVoidElement` | `(tag: string): bool` | Detect all 14 HTML5 void elements (`area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `source`, `track`, `wbr`) |
| `HastTree` | `struct` + `.new` / `.addElement` / `.addText` / `.addComment` / `.addDoctype` / `.setAttr` / `.toHtml` / `.toText` / `.nodeCount` / `.countByTag` | Fixed-pool zero-alloc HAST tree (up to 64 nodes + 64 attributes) |

## Usage / 使用示例

```cp
import { HastTree } from "@cap/rehype-ast";

let tree = HastTree.new();
tree.addDoctype(0);
const html = tree.addElement(0, "html");
const body = tree.addElement(html as usize, "body");
const p = tree.addElement(body as usize, "p");
tree.setAttr(p as usize, "class", "lead");
tree.addText(p as usize, "Hello <CP> & World!");

let out: [512]u8 = undefined;
const n = tree.toHtml(out[0..512]);
// <!doctype html><html><body><p class="lead">Hello &lt;CP&gt; &amp; World!</p></body></html>
```
