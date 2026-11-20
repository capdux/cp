# CP 标准库：多语言词法高亮引擎手册 (`cap:highlight`)

## 一、 架构总览与设计准则

`cap:highlight` 是 CP 语言官方内置的纯 CP 多语言代码词法高亮标准库，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP CLI 代码审查、文档静态生成（配合 `cap:md` / `cap:diff` / `cap:tui`）与 AI Coding Agent 终端输出设计。支持 `import { ... } from "cap:highlight"` 与 `import cap from "cap"; cap.highlight.*` 动态挂载。

### 1. 支持语言与模块拓扑

- **支持 8 大语言**：`CP`、`Zig`、`Rust`、`TypeScript`、`Python`、`JSON`、`SQL`、`Bash`。
- **无损词法切分**：对任意源码输入（含空白、换行、注释、UTF-8 文本与特殊符号），`concat(tokens) == code` 恒成立。

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/highlight/highlight.cp` | `// Source: cp/lib/highlight/highlight.cp (...)` | `HighlightError`、`Language`、`TokenScope`、`HighlightToken`、`Highlighter` |
| `lib/highlight/index.cp` | `// Source: cp/lib/highlight/index.cp (...)` | `cap:highlight` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与枚举类型

- **`HighlightError`**：`UnsupportedLanguage`、`OutOfMemory`。
- **`Language`**：`Cp`、`Zig`、`Rust`、`TypeScript`、`Python`、`Json`、`Sql`、`Bash`；提供实例方法 `name(): string`。
- **`TokenScope`**：`Plain`、`Keyword`、`TypeName`、`String`、`Number`、`Comment`、`Operator`、`Punctuation`、`Function`、`Builtin`；提供实例方法 `cssClass(): string`（`"hl-*"`）与 `ansiCode(): string`。
- **`HighlightToken`**：`scope: TokenScope`、`text: string`（零拷贝指向原源码切片）。

### 2. `Highlighter` 静态引擎

- **`Highlighter.languageFromName(name_or_ext: string): Language?`**：大小写不敏感解析语言名称或文件扩展名（如 `"cp"`、`".zig"`、`"rs"`、`"ts"`、`"py"`、`"json"`、`"sql"`、`"sh"`/`"bash"`），未知返回 `null`。
- **`Highlighter.tokenize(alloc, lang, code): ([]HighlightToken) ! HighlightError`**：无损词法切分，识别单行注释（`//`、`#`、`--`）、块注释（`/* ... */`）、双/单/反引号字符串及转义（含 Python 三引号）、十六进制/二进制/八进制/浮点/科学计数数字、`@builtin` 注解、函数调用与语言专属关键字/类型表。
- **`Highlighter.toAnsi(alloc, lang, code): ([]u8) ! HighlightError`**：输出带 ANSI SGR 颜色序列的终端高亮文本，每个非 `Plain` token 后严格追加 `\x1b[0m` 重置；剥离 ANSI 后与原源码逐字节一致。
- **`Highlighter.toHtml(alloc, lang, code): ([]u8) ! HighlightError`**：输出 `<pre class="cp-hl"><code>...</code></pre>`，非 `Plain` token 包裹为 `<span class="hl-...">...</span>`，并对 `&`、`<`、`>`、`"`、`'` 执行严格 HTML 实体转义以防 XSS。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Highlighter, Language } from "cap:highlight";
import cap from "cap";

pub fn demoHighlight(): ! {
    const alloc = std.heap.page_allocator;
    const code = "pub fn add(a: i32, b: i32): i32 { return a + b; }\n";

    // 1. 无损词法切分
    const lang = Highlighter.languageFromName(".cp") ?? Language.Cp;
    const tokens = Highlighter.tokenize(alloc, lang, code)?;
    defer alloc.free(tokens);

    // 2. 终端 ANSI 高亮与 XSS 安全 HTML 输出
    const ansi = Highlighter.toAnsi(alloc, lang, code)?;
    defer alloc.free(ansi);

    const html = cap.highlight.Highlighter.toHtml(alloc, lang, code)?;
    defer alloc.free(html);
}
```
