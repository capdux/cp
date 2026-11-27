# @web/swagger — AI Agent Quick Ref

Official OpenAPI 3.1.0 & interactive Scalar/Swagger UI documentation plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SwaggerConfig,
    swAsciiLower,
    swAsciiEqlIgnoreCase,
    swWriteByte,
    swWriteRaw,
    swWriteHtmlEscaped,
    renderDocsHtml,
    refreshSpec,
} from "@web/swagger";
```

## Core Types & Constants

```cp
class SwaggerConfig { title: string, version: string, spec_path: string, docs_path: string, /* +1 fields */ }
```

## API Signatures

```cp
fn swAsciiLower(c: u8): u8;
fn swAsciiEqlIgnoreCase(a: string, b: string): bool;
fn swWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn swWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn swWriteHtmlEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
fn renderDocsHtml(cfg: SwaggerConfig, out_buf: []u8): usize?;
fn refreshSpec(app: &mut App): usize?;
fn cachedSpec(): string;
fn cachedDocsHtml(): string;
fn swOpenApiSpecHandler(ctx: &mut Context): Response;
fn swDocsUiHandler(ctx: &mut Context): Response;
fn registerSwagger(app: &mut App, cfg: SwaggerConfig): &mut App;
```

## Minimal Usage

```cp
import { swAsciiEqlIgnoreCase } from "@web/swagger";

pub fn main() {
    let result = swAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/swagger"`; install via `cap add @web/swagger`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
