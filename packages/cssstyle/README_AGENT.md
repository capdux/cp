# cssstyle — AI Agent Quick Ref

Zero-allocation CSS inline style declaration parser, query, mutator, and serializer (npm cssstyle parity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CssProp,
    CstParsedValue,
    CssStyleDecl,
    cstSkipSpaceAndComments,
    cstStripTrailingComments,
    cstExtractImportant,
} from "cssstyle";
```

## Core Types & Constants

```cp
class CssProp { name: string, value: string, important: bool }
class CstParsedValue { value: string, important: bool }
class CssStyleDecl { props: [64]CssProp, count: usize } // methods: new(...) +7
```

## API Signatures

```cp
fn cstSkipSpaceAndComments(s: string, pos: usize): usize;
fn cstStripTrailingComments(s: string): string;
fn cstExtractImportant(raw: string): CstParsedValue;
```

## Minimal Usage

```cp
import { cstSkipSpaceAndComments } from "cssstyle";

pub fn main() {
    let result = cstSkipSpaceAndComments("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cssstyle"`; install via `cap add cssstyle`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
