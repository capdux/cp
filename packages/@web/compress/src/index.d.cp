// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class CompressConfig {
    min_bytes: usize,
    prefer_gzip: bool,
}

export enum EncodingKind {
    None,
    Gzip,
    Deflate
}

export fn cmpAsciiLower(c: u8): u8;
export fn cmpEqlIgnoreCase(a: string, b: string): bool;
export fn cmpTrimSpaces(s: string): string;
export fn cmpParseQValue(raw: string): i32;
export fn negotiateEncoding(accept_encoding: string, prefer_gzip: bool): EncodingKind;
export fn cmpCrc32(data: string): u32;
export fn cmpWriteU16Le(out_buf: []u8, pos: usize, val: u16);
export fn cmpWriteU32Le(out_buf: []u8, pos: usize, val: u32);
export fn encodeGzipStored(src: string, out_buf: []u8): ?usize;
export fn compressResponseHook(ctx: &mut Context, resp: &mut Response);
export fn createCompressPlugin(cfg: CompressConfig): Plugin;
export fn registerCompress(app: &mut App, cfg: CompressConfig): &mut App;
