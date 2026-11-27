[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# mustache

纯 CP 零堆分配 Mustache v1.4 无逻辑模板引擎，内置跨包复用 `escape-html` 自动防 XSS 转义 —— 洁净室 MIT 实现，对标 npm `mustache` v4。

支持 `{{ var }}`（HTML 自动转义）、`{{{ var }}}` / `{{& var }}`（原始输出）、`{{# section }}...{{/ section }}`（真值条件与列表重复）、`{{^ section }}...{{/ section }}`（假值反转区块）与 `{{! comment }}`（注释剔除），直接渲染到调用方栈缓冲区，全程零堆分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `mustache`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/janl/mustache.js` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: mustache)
import Mustache from "mustache";
const html = Mustache.render("Hello, {{name}}!", { name: "<b>CP</b>" });
```

```cp
// CP (cap: mustache) — 零堆分配直接渲染到栈缓冲
import { MustacheContext, renderMustache } from "mustache";

let ctx = MustacheContext.new();
ctx.setStr("name", "<b>CP</b>");
let out: [256]u8 = undefined;
const n = renderMustache("Hello, {{name}}!", ctx, out[0..256]);
// out[0..n] == "Hello, &lt;b&gt;CP&lt;/b&gt;!"
```

## 安装

```bash
cap add mustache
```

## 使用示例

### 基础用法：防 XSS 转义、原始插值、条件区块与注释

```cp
import { MustacheContext, renderMustache } from "mustache";

fn demoRender() {
    let ctx = MustacheContext.new();
    ctx.setStr("user", "<script>alert(1)</script>");
    ctx.setStr("badge", "<strong>PRO</strong>");
    ctx.setBool("is_admin", true);
    ctx.setBool("is_guest", false);

    const tpl = "{{! header }}Hi {{user}} {{{badge}}}{{#is_admin}} [ADMIN]{{/is_admin}}{{^is_guest}} (signed in){{/is_guest}}";
    let out: [512]u8 = undefined;
    const len = renderMustache(tpl, ctx, out[0..512]);
    // {{user}} 自动转义防 XSS，{{{badge}}} 原样输出
    _ = len;
}
```

### 边界用法：列表重复渲染、小缓冲安全截断与池容量

```cp
import { MustacheContext, renderMustache } from "mustache";

fn demoEdgeCases() {
    let ctx = MustacheContext.new();
    // 使用 setListCount 控制区块重复次数
    ctx.setListCount("rows", 3);
    ctx.setListCount("empty_rows", 0);

    let out: [128]u8 = undefined;
    const n1 = renderMustache("{{#rows}}<li>item</li>{{/rows}}{{^empty_rows}}none{{/empty_rows}}", ctx, out[0..128]);
    // => "<li>item</li><li>item</li><li>item</li>none"

    // 小缓冲区自动安全截断，绝不越界或 Panic；未绑定键渲染为空串
    let tiny: [8]u8 = undefined;
    const n2 = renderMustache("Hello {{missing_key}} world!", ctx, tiny[0..8]);
    // n2 <= 8

    // 上下文固定池容量为 32 项（超出时 set* 返回 false）
    const count = ctx.entryCount(); // 2
    _ = n1;
    _ = n2;
    _ = count;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `MustacheContext.new` | `fn(): MustacheContext` | 创建空栈上上下文（固定容量 32 项） |
| `MustacheContext.setStr` | `fn(*_, key: string, val: string): bool` | 设置/更新字符串键（键上限 48B，值上限 128B），池满返回 `false` |
| `MustacheContext.setBool` | `fn(*_, key: string, val: bool): bool` | 设置/更新布尔值，用于 `{{#...}}` / `{{^...}}` 条件判断 |
| `MustacheContext.setListCount` | `fn(*_, key: string, list_count: usize): bool` | 设置/更新列表重复次数（`0` 视为假值，`>0` 重复渲染对应次数） |
| `MustacheContext.entryCount` | `fn(): usize` | 返回当前已绑定条目数量（`0..32`） |
| `renderMustache` | `fn(template: string, ctx: MustacheContext, out: []u8): usize` | 渲染模板至 `out` 缓冲区，返回实际写入字节数 |
| `mstLookup` | `fn(ctx: MustacheContext, key: string): i32` | 按键查找条目索引（未找到返回 `-1`） |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`strEq`、`trim`）与 `cap:fmt`（`writeStr`）加速标签解析与缓冲写入。
- **零堆分配与固定栈池**：`MustacheContext` 内置 `[32]MustacheEntry` 栈池，无动态内存分配与 GC 抖动。
- **`undefined` 零清零优化**：条目缓冲区（`key`、`str_val`、`entries`）采用 `undefined` 声明，避免空闲槽位的无谓清零开销。
- **默认防 XSS 与边界保护**：`{{ key }}` 自动经 `escape-html` 转义；所有写入受 `out.len` 严格边界校验，小缓冲安全截断。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `mustache`。
