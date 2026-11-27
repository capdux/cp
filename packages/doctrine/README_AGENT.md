# doctrine — AI Agent Quick Ref

Zero-allocation JSDoc comment parser extracting description and @param/@returns/@type/@deprecated/@throws/@template tags (npm doctrine parity) for CP (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    JsDocTag,
    JsDocComment,
    docIsSpace,
    docStripLinePrefix,
    docTagHasName,
    docParseTagLine,
    parseJsDoc,
} from "doctrine";
```

## Core Types & Constants

```cp
class JsDocTag { title: string, type_expr: string, name: string, description: string, /* +1 fields */ }
class JsDocComment { description: string, tags: [16]JsDocTag, tag_count: usize } // methods: tagCount(...) +3
```

## API Signatures

```cp
fn docIsSpace(ch: u8): bool;
fn docStripLinePrefix(line: string): string;
fn docTagHasName(title: string): bool;
fn docParseTagLine(line: string): JsDocTag;
fn parseJsDoc(comment: string): JsDocComment;
```

## Minimal Usage

```cp
import { parseJsDoc } from "doctrine";

pub fn main() {
    let result = parseJsDoc("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"doctrine"`; install via `cap add doctrine`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
