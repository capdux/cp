[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# postcss

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:postcss`](https://github.com/postcss/postcss)（MIT，v8 语义）| CP 包版本：`1.0.0` | 零外部依赖

零堆分配的 CSS AST 解析、查询/修改与格式化序列化引擎。支持普通规则、嵌套 `@media` / `@supports` / `@keyframes` 块级 At-Rule、语句型 At-Rule（`@import`、`@charset`）、`/* ... */` 注释、`!important` 标记、声明检索与原地修改。

## 零成本迁移对照（TypeScript / JS → CP）

直接沿用原包名 `"postcss"`，无需改名即可完成迁移：

| TypeScript / Node.js (`npm:postcss`) | CP (`postcss`) |
| :--- | :--- |
| `import postcss from "postcss";` | `import { PostCssSheet } from "postcss";` |
| `const root = postcss.parse(css);` | `let sheet = PostCssSheet.parse(css);` |
| `root.walkDecls("color", ...)` | `sheet.findDecl(".btn", "color", out)` |
| `decl.value = "red"; decl.important = true;` | `sheet.setDecl(".btn", "color", "red", true)` |
| `root.toString()` | `sheet.toCss(out)` |

## 核心 API

### 导出常量

- 节点类型：`PC_RULE: u8`、`PC_DECL: u8`、`PC_AT_RULE: u8`、`PC_COMMENT: u8`
- 池容量上限：`PC_MAX_NODES: usize = 32`、`PC_MAX_DECLS: usize = 64`、`PC_MAX_COMMENTS: usize = 16`

### 类型与方法

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `PostCssSheet.empty(): PostCssSheet` | 创建空样式表 AST。 |
| `PostCssSheet.parse(css: string): PostCssSheet` | 解析 CSS 文本为扁平栈上 AST（语法有效时 `sheet.ok == true`）。 |
| `sheet.ruleCount()` / `declCount()` / `atRuleCount()` / `commentCount()` | 统计规则、声明、At-Rule 与注释数量。 |
| `sheet.findDecl(selector: string, prop: string, out: []u8): i64` | 查找声明值并写入 `out`，命中返回字节数（`>= 0`），未找到返回 `-1`。 |
| `sheet.isDeclImportant(selector: string, prop: string): bool` | 判断 `(selector, prop)` 是否带有 `!important`。 |
| `sheet.setDecl(*_, selector, prop, val, important: bool): bool` | 更新已有声明或新增规则/声明，池满时返回 `false`。 |
| `sheet.toCss(out: []u8): usize` | 将 AST 序列化为缩进 CSS 文本写入 `out`，返回写入字节数。 |
| `PostCssNode` / `PostCssDecl` / `PostCssComment` | 扁平 AST 节点类型，提供 `nameStr()` / `paramsStr()` / `propStr()` / `valueStr()` / `textStr()`。 |

## 实战示例

### 解析、查询与修改 CSS 声明

```cp
import { PostCssSheet } from "postcss";

fn demoCssTransform() {
    const css = "/* theme */\n.btn { color: #333; padding: 8px 16px !important; }\n@media (max-width: 600px) { .btn { padding: 4px; } }";
    let sheet = PostCssSheet.parse(css);

    if (sheet.ok) {
        let val_buf: [128]u8 = undefined;
        const n = sheet.findDecl(".btn", "color", val_buf[0..]);
        // val_buf[0..n] == "#333"

        const imp = sheet.isDeclImportant(".btn", "padding"); // true
        * = sheet.setDecl(".btn", "color", "#0055ff", false);
        * = sheet.setDecl(".btn", "border-radius", "6px", false);

        let out: [512]u8 = undefined;
        const css_len = sheet.toCss(out[0..]);
        * = n; * = imp; * = css_len;
    }
}
```

### 异常 CSS 安全拦截

```cp
import { PostCssSheet } from "postcss";

fn demoMalformed() {
    // 括号未闭合或字符串未闭合时安全置 ok = false，不触发 panic
    const bad = PostCssSheet.parse(".card { color: red;");
    // bad.ok == false
    * = bad.ok;
}
```

## 性能与安全亮点

- **零堆分配**：扁平定长槽位池（`nodes: [32]`、`decls: [64]`、`comments: [16]`）配合 `undefined` 初始化，零动态分配且跳过初始化清零。
- **内联安全所有权**：选择器、属性名（`[48]u8`）与属性值（`[128]u8`）均内联拷贝存储，修改与序列化绝无悬空切片风险。
- **父索引扁平树**：嵌套 `@media`/`@supports` 仅用 `parent_node: i32` 索引关联，无指针追踪与内存泄漏。

## 许可协议

MIT
