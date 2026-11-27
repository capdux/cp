# ejs — AI Agent Quick Ref

Zero-heap EJS template engine supporting <%= escaped %>, <%- raw %>, <%# comments %>, <% if/else %>, and <% repeat %> tags (clean-room port of npm ejs) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EjsEntry,
    EjsContext,
    ejsWriteStr,
    ejsLookupStr,
    ejsLookupBool,
    ejsLookupCount,
    ejsFindTagOpen,
    ejsFindTagClose,
} from "ejs";
```

## Core Types & Constants

```cp
class EjsEntry { key: string, str_val: string, bool_val: bool, count_val: usize, /* +1 fields */ }
class EjsContext { entries: [32]EjsEntry, count: usize } // methods: new(...) +4
```

## API Signatures

```cp
fn ejsWriteStr(s: string, out: []u8, pos: usize): usize;
fn ejsLookupStr(ctx: EjsContext, key: string): string;
fn ejsLookupBool(ctx: EjsContext, key: string): bool;
fn ejsLookupCount(ctx: EjsContext, key: string): usize;
fn ejsFindTagOpen(tpl: string, start_pos: usize): usize;
fn ejsFindTagClose(tpl: string, start_pos: usize): usize;
fn ejsFindBlockBounds(tpl: string, body_start: usize, out_pos: []usize): bool;
fn ejsRenderSlice(tpl: string, ctx: EjsContext, out: []u8, start_out: usize, depth: usize): usize;
fn renderEjs(tpl: string, ctx: EjsContext, out: []u8): usize;
```

## Minimal Usage

```cp
import { ejsWriteStr } from "ejs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ejsWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ejs"`; install via `cap add ejs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
