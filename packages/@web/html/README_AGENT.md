# @web/html@1.0.0 (Agent Quickref)

> Official HTML SSR, zero-heap XSS escaping, and HTMX helpers plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/html";` | Full types: `src/index.d.cp`

## Functions
- `export fn htmAsciiLower(c: u8): u8;`
- `export fn htmAsciiEqlIgnoreCase(a: string, b: string): bool;`
- `export fn htmWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn htmWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn htmEscapeInto(out_buf: []u8, pos: &mut usize, input: string): bool;`
- `export fn htmlEscape(input: string, out_buf: []u8): usize?;`
- `export fn htmlIsHtmx(ctx: &Context): bool;`
- `export fn htmlHxRedirect(url: string): Response;`
- `export fn htmlHxTrigger(body_html: string, event_name: string): Response;`
- `export fn htmAfterHook(ctx: &mut Context, resp: &mut Response);`
- `export fn registerHtml(app: &mut App, cfg: HtmlConfig): &mut App;`
