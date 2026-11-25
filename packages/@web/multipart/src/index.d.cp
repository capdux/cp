// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class MultipartConfig {
    max_parts: usize,
    max_file_bytes: usize,
    max_total_bytes: usize,
    allowed_mime_prefix: string,
}

export class MultipartPart {
    name: string,
    filename: string,
    content_type: string,
    data: string,
    is_file: bool,
}

export class MultipartForm {
    parts: MultipartPart[16],
    count: usize,
    total_bytes: usize,
}

export fn mpAsciiLower(c: u8): u8;
export fn mpStartsWithIgnoreCase(s: string, prefix: string): bool;
export fn mpEqlIgnoreCase(a: string, b: string): bool;
export fn mpTrimSpaces(s: string): string;
export fn mpParseUsize(s: string): ?usize;
export fn mpMatchAt(haystack: string, pos: usize, needle: string): bool;
export fn mpFindBoundary(body: string, start_pos: usize, boundary: string): ?usize;
export fn mpFindHeaderEnd(part: string, body_start_out: &mut usize): ?usize;
export fn mpExtractParam(header_val: string, key: string): ?string;
export fn parseMultipartBody(boundary: string, body: string, cfg: MultipartConfig, out_form: &mut MultipartForm): i32;
export fn multipartGuardHook(ctx: &mut Context): bool;
export fn createMultipartPlugin(cfg: MultipartConfig): Plugin;
export fn registerMultipart(app: &mut App, cfg: MultipartConfig): &mut App;
