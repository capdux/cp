# `cap:template` — 纯 CP Mustache 模板引擎（AI Prompt / HTML / 代码生成）

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

> **模块路径**: `lib/template/`  
> **导入协议**: `import { TemplateEngine, TemplateContext, TemplateValue, TemplateEntry, TemplateError } from "cap:text";`  
> **动态挂载**: `import cap from "cap";` -> `cap.template.*` 编译期按需惰性挂载

---

## 一、 架构概述与设计亮点

`cap:template` 是专为 **AI Prompt 编排、HTML/邮件渲染与代码生成（Codegen）** 打造的纯 CP Mustache 兼容模板引擎，采用“单遍字节码式编译（`CompiledTemplate`） + 栈式上下文解析（`TemplateContext`）”设计：

1. **零字符串拷贝指令切片（`TemplateOp`）**：`compile()` 将模板一次性解析为扁平操作序列（`Text` / `VarEscaped` / `VarRaw` / `SectionStart` / `InvertedStart` / `SectionEnd` / `Partial`），全部文本与变量路径直接借用源模板切片，并在编译阶段完成闭合标签与段名匹配校验。
2. **Mustache 完整语法兼容**：
   - `{{var}}`：默认自动转义 HTML 特殊字符（`&`, `<`, `>`, `"`, `'`）；
   - `{{{raw}}}` 与 `{{&raw}}`：局部关闭转义，原样输出；
   - `{{#section}}...{{/section}}`：支持布尔条件、非空值判定、列表遍历（`{{.}}`）与 Map 作用域压栈；
   - `{{^inverted}}...{{/inverted}}`：假值/空列表/空字符串/0/null 反向渲染；
   - `{{! comment }}`：注释忽略（支持独立行空白折叠）；
   - `{{> partial_name}}`：子模板注册、嵌套组合与最大递归深度防护（默认 `DEFAULT_MAX_DEPTH = 32`）。
3. **AI Prompt 与代码生成专属 `renderRaw()`**：支持全局关闭 HTML 转义，完美保留 `<xml>` 标签、`&&`、引号与代码块符号。
4. **完备语法与运行时错误诊断 (`TemplateError`)**：精准检测 `UnclosedTag`、`UnclosedSection`、`UnexpectedClosingSection`、`MismatchedSection`、`EmptyTag`、`PartialNotFound` 与 `MaxDepthExceeded`。

---

## 二、 核心 API 速查表

### 1. 数据模型与上下文 (`lib/template/value.cp`)

| 类型 / 方法 | 签名与说明 |
| :--- | :--- |
| `TemplateValueTag` | `Null`, `Bool`, `Int`, `Float`, `Str`, `List`, `Map` |
| `TemplateEntry` | `TemplateEntry.init(key, value)`，Map 键值对条目 |
| `TemplateValue` 构造工厂 | `fromNull()`, `fromBool(v)`, `fromInt(v)`, `fromFloat(v)`, `fromStr(s)`, `fromList(items)`, `fromMap(entries)` |
| `TemplateValue.isTruthy()` | `Null`/`false`/`0`/`0.0`/`NaN`/`""`/`[]` 为 `false`，其余（含 `Map`）为 `true` |
| `TemplateValue.getPath(path)` | 支持 `.`、`user.profile.name` 及列表索引 `items.0.title` 点分路径解析 |
| `TemplateContext` | `init(data)`、`initEmpty()`、`withParent(data, &parent)`、`resolve(path)`、`has(path)`、`isTruthy(path)` |

### 2. 模板引擎与编译产物 (`lib/template/engine.cp`)

| 方法 | 说明 |
| :--- | :--- |
| `TemplateEngine.init(allocator)` / `initDefault()` | 初始化模板引擎（默认 `max_depth = 32`, `escape_html = true`） |
| `deinit(*_)` | 释放已注册 Partial 的全部自有内存 |
| `registerPartial(*_, name, src)` | 注册或覆盖子模板（内部深拷贝存储，安全持久） |
| `setMaxDepth(*_, depth)` | 设置 Partial 最大递归深度上限 |
| `setEscapeHtml(*_, enabled)` | 配置默认 `render()` 是否启用 HTML 转义 |
| `setAllowMissingPartials(*_, allow)` | 配置遇到未注册 Partial 时报错（默认 `false`）或静默跳过 |
| `compile(src)` | 编译并校验语法，返回 `CompiledTemplate ! TemplateError` |
| `render(src, ctx)` | 按 `escape_html` 配置编译并渲染，返回调用方持有的 `[]u8` |
| `renderRaw(src, ctx)` | 全局关闭 HTML 转义渲染（AI Prompt / Markdown / Codegen 首选） |
| `renderCompiled(&tmpl, ctx)` / `renderCompiledRaw(&tmpl, ctx)` | 复用已编译 `CompiledTemplate` 零重解析渲染 |

---

## 三、 代码示例

```typescript
const std = @import("std");
import { TemplateEngine, TemplateContext, TemplateValue, TemplateEntry } from "cap:text";

fn demoPromptAndHtml(): !void {
    const alloc = std.heap.page_allocator;
    let engine = TemplateEngine.init(alloc);
    defer engine.deinit();

    * = engine.registerPartial("header", "<h1>{{title}}</h1>")?;

    const user_fields = [
        TemplateEntry.init("name", TemplateValue.fromStr("Alice <Admin>")),
    ];
    const items = [
        TemplateValue.fromStr("read_file"),
        TemplateValue.fromStr("run_test"),
    ];
    const root_fields = [
        TemplateEntry.init("title", TemplateValue.fromStr("Agent Report")),
        TemplateEntry.init("user", TemplateValue.fromMap(&user_fields)),
        TemplateEntry.init("tools", TemplateValue.fromList(&items)),
    ];
    const ctx = TemplateContext.init(TemplateValue.fromMap(&root_fields));

    // HTML 安全渲染（自动转义 & < > " '）
    const html = engine.render("{{> header}}<p>{{user.name}}</p>", ctx)?;
    defer alloc.free(html);

    // AI Prompt 原始渲染（全局关闭转义）
    const prompt = engine.renderRaw("User: {{user.name}}\n{{#tools}}- {{.}}\n{{/tools}}", ctx)?;
    defer alloc.free(prompt);
}
```
