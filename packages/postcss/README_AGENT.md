# postcss@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:postcss` (`postcss/postcss`, MIT, v8 parity) | **Deps**: none
- **Import**: `import { PostCssSheet, PostCssNode, PostCssDecl, PostCssComment, PC_RULE, PC_DECL, PC_AT_RULE, PC_COMMENT, PC_MAX_NODES, PC_MAX_DECLS, PC_MAX_COMMENTS } from "postcss";`
- **Full Types**: `packages/postcss/src/index.d.cp`

## Constants & Pool Limits
```cp
PC_MAX_NODES:    usize = 32  // name: [64]u8, params: [128]u8
PC_MAX_DECLS:    usize = 64  // prop: [48]u8, value:  [128]u8
PC_MAX_COMMENTS: usize = 16  // text: [128]u8
```

## Core Types & Methods
```cp
class PostCssDecl    { kind: u8, parent_node: i32, prop: [48]u8, prop_len: usize, value: [128]u8, value_len: usize, important: bool; fn propStr(): &[]u8; fn valueStr(): &[]u8 }
class PostCssNode    { kind: u8, parent_node: i32, name: [64]u8, name_len: usize, params: [128]u8, params_len: usize, has_block: bool; fn nameStr(): &[]u8; fn paramsStr(): &[]u8 }
class PostCssComment { kind: u8, text: [128]u8, text_len: usize; fn textStr(): &[]u8 }

class PostCssSheet {
    nodes: [32]PostCssNode, node_len: usize,
    decls: [64]PostCssDecl, decl_len: usize,
    comments: [16]PostCssComment, comment_len: usize,
    ok: bool,
    fn empty(): PostCssSheet
    fn parse(css: string): PostCssSheet
    fn ruleCount(): usize; fn declCount(): usize; fn atRuleCount(): usize; fn commentCount(): usize
    fn findDecl(selector: string, prop: string, out: []u8): i64
    fn isDeclImportant(selector: string, prop: string): bool
    fn setDecl(_: auto, selector: string, prop: string, val: string, important: bool): bool
    fn toCss(out: []u8): usize
}
```

## Minimal Pattern
```cp
import { PostCssSheet } from "postcss";

let sheet = PostCssSheet.parse(".btn { color: red !important; }");
if (sheet.ok) {
    * = sheet.setDecl(".btn", "color", "blue", false);
    let out: [256]u8 = undefined;
    const n = sheet.toCss(out[0..]);
}
```

## Pitfalls & Notes
1. **Check `sheet.ok`**: Unclosed `{...}`, strings, or comments set `sheet.ok = false`.
2. **Return Type of `findDecl`**: Returns `i64` (`>= 0` bytes written into `out`, or `-1` if selector/prop not found).
3. **Mutable Receiver for `setDecl`**: Declare `let sheet = PostCssSheet.parse(...)`; `setDecl` returns `false` if `PC_MAX_NODES` (32) or `PC_MAX_DECLS` (64) is exhausted.
4. **Field Truncation**: Selector names clamp to 64 B, props to 48 B, values/params/comments to 128 B.
5. **Internal Helpers**: `pc*` functions are not in `export { ... }` — use `PostCssSheet` methods.
