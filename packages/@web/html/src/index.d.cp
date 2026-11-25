// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class HtmlConfig {
    default_charset: string,
}

export fn htmAsciiLower(c: u8): u8;
export fn htmAsciiEqlIgnoreCase(a: string, b: string): bool;
export fn htmWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn htmWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn htmEscapeInto(out_buf: []u8, pos: &mut usize, input: string): bool;
export fn htmlEscape(input: string, out_buf: []u8): usize?;
export class HtmlBuilder {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn init(buf: []u8): HtmlBuilder;
    fn raw(_: auto, s: string): &mut HtmlBuilder;
    fn text(_: auto, s: string): &mut HtmlBuilder;
    fn tag(_: auto, name: string, content: string): &mut HtmlBuilder;
    fn tagRaw(_: auto, name: string, inner_html: string): &mut HtmlBuilder;
    fn slice(): string;
    fn len(): usize;
    fn isOverflow(): bool;
    fn reset(_: auto);
}

export fn htmlIsHtmx(ctx: &Context): bool;
export fn htmlHxRedirect(url: string): Response;
export fn htmlHxTrigger(body_html: string, event_name: string): Response;
export fn htmAfterHook(ctx: &mut Context, resp: &mut Response);
export fn registerHtml(app: &mut App, cfg: HtmlConfig): &mut App;
