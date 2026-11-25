// Type definitions for CP

import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
export class SwaggerConfig {
    title: string,
    version: string,
    spec_path: string,
    docs_path: string,
    ui_theme: string,
}

export fn swAsciiLower(c: u8): u8;
export fn swAsciiEqlIgnoreCase(a: string, b: string): bool;
export fn swWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn swWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn swWriteHtmlEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn renderDocsHtml(cfg: SwaggerConfig, out_buf: []u8): ?usize;
export fn refreshSpec(app: &mut App): ?usize;
export fn cachedSpec(): string;
export fn cachedDocsHtml(): string;
export fn swOpenApiSpecHandler(ctx: &mut Context): Response;
export fn swDocsUiHandler(ctx: &mut Context): Response;
export fn registerSwagger(app: &mut App, cfg: SwaggerConfig): &mut App;
