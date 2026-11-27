[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# postcss

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:postcss`](https://github.com/postcss/postcss) (MIT, v8 parity) | CP Package Version: `1.0.0` | Zero external dependencies

Zero-heap CSS AST parser, query/mutation engine, and indented serializer for CP. Supports rules, nested `@media` / `@supports` / `@keyframes` at-rules, statement at-rules (`@import`, `@charset`), `/* ... */` comments, `!important` flags, declaration lookup/upsert, and CSS serialization.

## Zero-Cost Migration (TypeScript / JS → CP)

Keep the exact same package name (`"postcss"`) — no renaming required.

| TypeScript / Node.js (`npm:postcss`) | CP (`postcss`) |
| :--- | :--- |
| `import postcss from "postcss";` | `import { PostCssSheet } from "postcss";` |
| `const root = postcss.parse(css);` | `let sheet = PostCssSheet.parse(css);` |
| `root.walkDecls("color", ...)` | `sheet.findDecl(".btn", "color", out)` |
| `decl.value = "red"; decl.important = true;` | `sheet.setDecl(".btn", "color", "red", true)` |
| `root.toString()` | `sheet.toCss(out)` |

## API Reference

### Constants

- Node kinds: `PC_RULE: u8`, `PC_DECL: u8`, `PC_AT_RULE: u8`, `PC_COMMENT: u8`
- Pool limits: `PC_MAX_NODES: usize = 32`, `PC_MAX_DECLS: usize = 64`, `PC_MAX_COMMENTS: usize = 16`

### Core Types & Methods

| Symbol / Signature | Description |
| :--- | :--- |
| `PostCssSheet.empty(): PostCssSheet` | Create an empty stylesheet AST. |
| `PostCssSheet.parse(css: string): PostCssSheet` | Parse CSS text into a flat stack-allocated AST (`sheet.ok == true` if valid). |
| `sheet.ruleCount()` / `declCount()` / `atRuleCount()` / `commentCount()` | Query counts of parsed rules, declarations, at-rules, and comments. |
| `sheet.findDecl(selector: string, prop: string, out: []u8): i64` | Copy declaration value into `out`; returns byte length (`>= 0`) or `-1` if missing. |
| `sheet.isDeclImportant(selector: string, prop: string): bool` | Check whether `(selector, prop)` has `!important`. |
| `sheet.setDecl(*_, selector, prop, val, important: bool): bool` | Update existing declaration or append new rule/declaration; returns `false` if pool full. |
| `sheet.toCss(out: []u8): usize` | Serialize AST back to formatted CSS into `out`; returns bytes written. |
| `PostCssNode` / `PostCssDecl` / `PostCssComment` | Flat AST node records with `nameStr()` / `paramsStr()` / `propStr()` / `valueStr()` / `textStr()`. |

## Usage Examples

### Parse, Query & Mutate CSS Declarations

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

### Handling Malformed CSS Safely

```cp
import { PostCssSheet } from "postcss";

fn demoMalformed() {
    // Unclosed brace or unclosed string sets sheet.ok = false without panic
    const bad = PostCssSheet.parse(".card { color: red;");
    // bad.ok == false
    * = bad.ok;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: Flat inline pools (`nodes: [32]PostCssNode`, `decls: [64]PostCssDecl`, `comments: [16]PostCssComment`) initialized with `undefined` — zero heap allocations and zero startup zeroing.
- **Inline Ownership**: Node names (`[64]u8`), params (`[128]u8`), declaration props (`[48]u8`), and values (`[128]u8`) are copied inline so mutations and serialization never hold dangling slices.
- **Parent-Index Tree**: Nested `@media`/`@supports` blocks link via `parent_node: i32` indices instead of heap pointers, eliminating pointer chasing and leaks.

## License

MIT
