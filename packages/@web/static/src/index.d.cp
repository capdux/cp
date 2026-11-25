// Type definitions for CP

import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import { detectFileType } from "../../../../lib/mime/fileType.cp";
export class StaticConfig {
    root_dir: string,
    url_prefix: string,
    index_file: string,
    cache_control: string,
    enable_etag: bool,
}

export fn stAsciiLower(c: u8): u8;
export fn stAsciiEqlIgnoreCase(a: string, b: string): bool;
export fn stTrimAsciiSpaces(s: string): string;
export fn stHexVal(c: u8): u8?;
export fn stHexDigitLower(n: u8): u8;
export fn stWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn stWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn sanitizeRelPath(raw: string, out_buf: []u8): usize?;
export fn detectMimeType(path: string, content: string): string;
export fn computeStrongEtag(content: string, out_buf: []u8): usize?;
export fn etagMatches(if_none_match: string, etag: string): bool;
export fn stReadRegularFileInto(full_path: string, out_buf: []u8): usize?;
export fn stJoinFsPath(root_dir: string, rel_path: string, out_buf: []u8): usize?;
export fn stServeWithEtagBuf(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8, etag_buf: []u8): Response;
export fn serveStaticRequest(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8): Response;
export fn serveFileFromDir(root_dir: string, rel_path: string, out_buf: []u8): Response;
export fn serveStaticIntoArena(ctx: &mut Context, cfg: StaticConfig, rel_path: string): Response;
export fn stRouteHandler(ctx: &mut Context): Response;
export fn registerStatic(app: &mut App, cfg: StaticConfig): &mut App;
