# jest-docblock — AI Agent Quick Ref

Zero-heap top-of-file `/** ... */` docblock extractor, stripper, `@pragma` parser, and formatter.

## Import
```typescript
import {
    DocPragma, ParsedDocblock,
    extract, extractDocblock, strip, stripSlice,
    parse, parseDocblock, parseWithComments,
    print, printDocblock, printWithComments,
    getPragma, hasPragma
} from "jest-docblock";
```

## API Signatures
```typescript
export class DocPragma { key: string, value: string }
export class ParsedDocblock { comments_len: usize, pragma_count: usize }

export fn extractDocblock(code: string): string;
export fn extract(code: string): string;
export fn strip(code: string, out: []u8): usize;
export fn stripSlice(code: string): string;
export fn parseDocblock(docblock: string, out: []DocPragma): usize;
export fn parse(docblock: string, out: []DocPragma): usize;
export fn parseWithComments(docblock: string, comments_out: []u8, pragmas_out: []DocPragma): ParsedDocblock;
export fn printDocblock(pragmas: &[]DocPragma, out: []u8): usize;
export fn print(pragmas: &[]DocPragma, out: []u8): usize;
export fn printWithComments(comments: string, pragmas: &[]DocPragma, out: []u8): usize;
export fn getPragma(pragmas: &[]DocPragma, key: string): string?;
export fn hasPragma(pragmas: &[]DocPragma, key: string): bool;
```

## Minimal Example
```typescript
import { extract, parse, getPragma, DocPragma } from "jest-docblock";

fn envOf(src: string): string {
    let doc = extract(src);
    let mut pragmas: [8]DocPragma = undefined;
    let n = parse(doc, &mut pragmas);
    return getPragma(pragmas[0..n], "jest-environment") orelse "node";
}
```

## Pitfalls & Rules
- `extract`/`stripSlice` return zero-copy slices tied to `code`'s lifetime; `DocPragma.key`/`value` borrow from `docblock`.
- `parse` / `parseWithComments` stop once `out` / `pragmas_out` is full; size the output array generously.
- Pass read-only `&[]DocPragma` slices (e.g. `pragmas[0..n]`) to `print`, `printWithComments`, `getPragma`, `hasPragma`.
- `extract` / `extractDocblock`, `parse` / `parseDocblock`, `print` / `printDocblock` are exact aliases.
- `getPragma` returns `string?`; handle with `orelse` or `if (v := getPragma(...))`.
