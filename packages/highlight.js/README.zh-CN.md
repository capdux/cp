[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# highlight.js

纯 CP 零堆分配多语言词法分析与语法高亮引擎，内置通用代码（CP/TS/Rust/C）、JSON 与 SQL 三大模式，支持 ANSI 终端彩色输出与防 XSS 转义 HTML `<span class="hl-...">` 输出 —— 洁净室 MIT 实现，对标 npm `highlight.js` 与 `prismjs`。

可将源码零拷贝切分为 `Token` 区间数组（`{ kind, start, len }`），或单遍直接渲染至调用方栈缓冲区，缓冲区不足时确定性返回 `-1`，全程零堆分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `highlight.js`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/highlightjs/highlight.js` |
| 许可证 | 上游 `BSD-3-Clause`（CP 洁净室实现：`MIT`） |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: highlight.js)
import hljs from "highlight.js";
const html = hljs.highlight("const x: i32 = 42;", { language: "typescript" }).value;
```

```cp
// CP (cap: highlight.js) — 零堆分配防 XSS HTML 与 ANSI 语法高亮
import { LANG_CODE, highlightHtml } from "highlight.js";

const src: []const u8 = "const x: i32 = 42;";
let out: [512]u8 = undefined;
const n = highlightHtml(&src, LANG_CODE, out[0..512]);
```

## 安装

```bash
cap add highlight.js
```

## 使用示例

### 基础用法：Code / JSON / SQL 的 HTML 与 ANSI 高亮

```cp
import {
    LANG_CODE,
    LANG_JSON,
    LANG_SQL,
    highlightHtml,
    highlightAnsi,
} from "highlight.js";

fn demoHighlight() {
    let html_buf: [512]u8 = undefined;
    let ansi_buf: [512]u8 = undefined;

    // 1. 防 XSS HTML 高亮（自动转义 <、>、&、"）
    const code: []const u8 = "fn add(a: i32, b: i32): i32 { return a + b; }";
    const hlen = highlightHtml(&code, LANG_CODE, html_buf[0..512]);
    // hlen >= 0 时结果为 html_buf[0..(hlen as usize)]

    // 2. 终端 ANSI 彩色高亮与 JSON 高亮
    const sql: []const u8 = "SELECT id, name FROM users WHERE id = 1;";
    const alen = highlightAnsi(&sql, LANG_SQL, ansi_buf[0..512]);

    const json: []const u8 = "{\"ok\": true, \"count\": 42}";
    const jlen = highlightHtml(&json, LANG_JSON, html_buf[0..512]);
    _ = hlen;
    _ = alen;
    _ = jlen;
}
```

### 边界用法：零拷贝 Token 切分与缓冲区溢出检测（`-1`）

```cp
import {
    Token,
    LANG_CODE,
    TOK_KEYWORD,
    tokenize,
    highlightHtml,
    htmlClassFor,
    ansiCodeFor,
} from "highlight.js";

fn demoTokensAndOverflow() {
    // 1. 零拷贝词法切分到栈上 Token 数组
    const src: []const u8 = "let count: usize = 10; // comment";
    let tokens: [32]Token = undefined;
    const count = tokenize(&src, LANG_CODE, tokens[0..32]);
    // tokens[i] 记录 { kind, start, len }，直接对应 src[start..(start+len)]

    // 2. 输出缓冲区不足时确定性返回 -1（绝不 Panic 或越界）
    let tiny: [8]u8 = undefined;
    const err_n = highlightHtml(&src, LANG_CODE, tiny[0..8]);
    // err_n == -1

    const cls = htmlClassFor(TOK_KEYWORD); // "hl-kw"
    const esc = ansiCodeFor(TOK_KEYWORD);
    _ = count;
    _ = err_n;
    _ = cls;
    _ = esc;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `LANG_*` | `LANG_CODE(0)`、`LANG_JSON(1)`、`LANG_SQL(2)` | 内置语言模式常量（`u8`） |
| `TOK_*` | `TOK_PLAIN(0)`、`TOK_KEYWORD(1)`、`TOK_STRING(2)`、`TOK_NUMBER(3)`、`TOK_COMMENT(4)`、`TOK_OPERATOR(5)`、`TOK_TYPE(6)`、`TOK_PUNCT(7)` | Token 类别常量（`u8`） |
| `Token` | `class { kind: u8, start: usize, len: usize }` | 零拷贝源码 Token 区间 |
| `tokenize` | `fn(src: &[]u8, lang: u8, out: []Token): usize` | 将源码切分为连续 `Token` 数组，返回写入条数 |
| `nextToken` | `fn(src: &[]u8, pos: usize, lang: u8): Token` | 从指定字节偏移 `pos` 读取下一个 Token |
| `highlightAnsi` | `fn(src: &[]u8, lang: u8, dst: []u8): i64` | 渲染 ANSI 彩色文本至 `dst`（溢出返回 `-1`） |
| `highlightHtml` | `fn(src: &[]u8, lang: u8, dst: []u8): i64` | 渲染防 XSS 转义的 `<span class="hl-...">` HTML（溢出返回 `-1`） |
| `ansiCodeFor` / `htmlClassFor` | `fn(kind: u8): string` | 查询 `TOK_*` 对应的 ANSI 转义前缀或 CSS 类名 |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`isSpace`、`isDigit`、`isHexDigit`、`isAlpha`、`strEq`、`eqIgnoreCase`）与 `cap:fmt`（`writeStr`）加速单遍词法扫描与高亮输出。
- **零堆分配栈上渲染**：全流程仅使用调用方提供的 `[]Token` 或 `[]u8` 缓冲区，无动态内存分配。
- **`undefined` 零清零栈缓冲**：支持传入 `undefined` 声明的栈缓冲，所有写入均执行严格边界检查。
- **XSS 安全与明确溢出信号**：`highlightHtml` 自动转义 `<`、`>`、`&`、`"`，缓冲不足时明确返回 `-1`。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `highlight.js`。
