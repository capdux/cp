// Official @web/static Plugin — Production Zero-GC Static File Server for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

const std = @import("std");
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import { detectFileType } from "../../../../lib/mime/fileType.cp";

pub const STATIC_MAX_PATH_LEN: usize = 1024;
pub const STATIC_MAX_FILE_BYTES: usize = 65536;
pub const STATIC_ETAG_BUF_LEN: usize = 64;

pub struct StaticConfig {
    root_dir: string = "./public",
    url_prefix: string = "/static",
    index_file: string = "index.html",
    cache_control: string = "public, max-age=3600",
    enable_etag: bool = true,
}

let g_static_cfg: StaticConfig = StaticConfig{};
let g_static_file_buf: [STATIC_MAX_FILE_BYTES]u8 = [0; STATIC_MAX_FILE_BYTES];
let g_static_etag_buf: [STATIC_ETAG_BUF_LEN]u8 = [0; STATIC_ETAG_BUF_LEN];

fn stAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn stAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (stAsciiLower(a[i]) != stAsciiLower(b[i])) return false;
    }
    return true;
}

fn stTrimAsciiSpaces(s: string): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn stHexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn stHexDigitLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn stWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn stWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

/// Strict path traversal guard and normalizer.
/// Rejects `..`, `.` segments, `\`, NUL bytes, control chars, and percent-encoded
/// traversal sequences (`%2e`, `%2f`, `%5c`, `%00`) by returning `null` (403 Forbidden).
pub fn sanitizeRelPath(raw: string, out_buf: []u8): usize? {
    // Pass 1: Reject dangerous characters and encoded traversal bytes.
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        const c = raw[i];
        if (c == 0 || c == '\\' || c < 0x20 || c == 0x7F) {
            return null;
        }
        if (c == '%') {
            if (i + 2 >= raw.len) return null;
            const hi = stHexVal(raw[i + 1]);
            const lo = stHexVal(raw[i + 2]);
            if (hi == null || lo == null) return null;
            const decoded: u8 = ((hi.?) << 4) | (lo.?);
            if (decoded == 0 || decoded == '.' || decoded == '/' || decoded == '\\' || decoded < 0x20 || decoded == 0x7F) {
                return null;
            }
        }
    }

    // Pass 2: Split by `/` and validate every segment, stripping leading/redundant slashes.
    let pos: usize = 0;
    let cursor: usize = 0;
    while (cursor < raw.len) {
        while (cursor < raw.len && raw[cursor] == '/') : (cursor += 1) {}
        if (cursor >= raw.len) break;

        const seg_start = cursor;
        while (cursor < raw.len && raw[cursor] != '/') : (cursor += 1) {}
        const seg = raw[seg_start..cursor];

        // Reject dot segments (`..` or `.`)
        if ((seg.len == 1 && seg[0] == '.') || (seg.len == 2 && seg[0] == '.' && seg[1] == '.')) {
            return null;
        }

        if (pos > 0) {
            if (!stWriteByte(out_buf, *pos, '/')) return null;
        }
        if (!stWriteRaw(out_buf, *pos, seg)) return null;
    }

    return pos;
}

/// Detect MIME type by file extension first, falling back to magic-number content probing.
pub fn detectMimeType(path: string, content: string): string {
    // Locate extension after the last `/`
    let dot_idx: usize? = null;
    let i: usize = path.len;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/') break;
        if (path[i] == '.') {
            dot_idx = i;
            break;
        }
    }

    if (d := dot_idx) {
        const ext = path[d..];
        if (stAsciiEqlIgnoreCase(ext, ".html") || stAsciiEqlIgnoreCase(ext, ".htm")) return "text/html; charset=utf-8";
        if (stAsciiEqlIgnoreCase(ext, ".css")) return "text/css; charset=utf-8";
        if (stAsciiEqlIgnoreCase(ext, ".js") || stAsciiEqlIgnoreCase(ext, ".mjs")) return "application/javascript; charset=utf-8";
        if (stAsciiEqlIgnoreCase(ext, ".json")) return "application/json; charset=utf-8";
        if (stAsciiEqlIgnoreCase(ext, ".svg")) return "image/svg+xml";
        if (stAsciiEqlIgnoreCase(ext, ".png")) return "image/png";
        if (stAsciiEqlIgnoreCase(ext, ".jpg") || stAsciiEqlIgnoreCase(ext, ".jpeg")) return "image/jpeg";
        if (stAsciiEqlIgnoreCase(ext, ".gif")) return "image/gif";
        if (stAsciiEqlIgnoreCase(ext, ".webp")) return "image/webp";
        if (stAsciiEqlIgnoreCase(ext, ".ico")) return "image/x-icon";
        if (stAsciiEqlIgnoreCase(ext, ".wasm")) return "application/wasm";
        if (stAsciiEqlIgnoreCase(ext, ".pdf")) return "application/pdf";
        if (stAsciiEqlIgnoreCase(ext, ".txt")) return "text/plain; charset=utf-8";
        if (stAsciiEqlIgnoreCase(ext, ".xml")) return "application/xml; charset=utf-8";
        if (stAsciiEqlIgnoreCase(ext, ".woff2")) return "font/woff2";
        if (stAsciiEqlIgnoreCase(ext, ".woff")) return "font/woff";
        if (stAsciiEqlIgnoreCase(ext, ".mp4")) return "video/mp4";
        if (stAsciiEqlIgnoreCase(ext, ".webm")) return "video/webm";
    }

    if (info := detectFileType(content)) {
        return info.mime;
    }

    return "application/octet-stream";
}

/// Compute a strong FNV-1a 64-bit ETag formatted as `"<16-hex>-<len-hex>"`.
pub fn computeStrongEtag(content: string, out_buf: []u8): usize? {
    let hash: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
    let i: usize = 0;
    while (i < content.len) : (i += 1) {
        hash = hash ^ (content[i] as u64);
        hash = hash *% 1099511628211;
    }

    let pos: usize = 0;
    if (!stWriteByte(out_buf, *pos, '"')) return null;

    let shift: u6 = 60;
    let nibble_idx: usize = 0;
    while (nibble_idx < 16) : (nibble_idx += 1) {
        const nib: u8 = ((hash >> shift) & 0x0F) as u8;
        if (!stWriteByte(out_buf, *pos, stHexDigitLower(nib))) return null;
        if (shift >= 4) {
            shift -= 4;
        }
    }

    if (!stWriteByte(out_buf, *pos, '-')) return null;

    // Encode content.len in lowercase hex
    let len_val: usize = content.len;
    if (len_val == 0) {
        if (!stWriteByte(out_buf, *pos, '0')) return null;
    } else {
        let tmp: [16]u8 = [0; 16];
        let n: usize = 0;
        while (len_val > 0) {
            tmp[n] = stHexDigitLower((len_val & 0x0F) as u8);
            n += 1;
            len_val = len_val >> 4;
        }
        while (n > 0) {
            n -= 1;
            if (!stWriteByte(out_buf, *pos, tmp[n])) return null;
        }
    }

    if (!stWriteByte(out_buf, *pos, '"')) return null;
    return pos;
}

/// Check whether an `If-None-Match` header matches the computed strong ETag.
pub fn etagMatches(if_none_match: string, etag: string): bool {
    const trimmed = stTrimAsciiSpaces(if_none_match);
    if (trimmed.len == 0) return false;
    if (trimmed.len == 1 && trimmed[0] == '*') return true;

    let pos: usize = 0;
    while (pos < trimmed.len) {
        let end: usize = pos;
        while (end < trimmed.len && trimmed[end] != ',') : (end += 1) {}
        let tag = stTrimAsciiSpaces(trimmed[pos..end]);
        if (tag.len >= 2 && tag[0] == 'W' && tag[1] == '/') {
            tag = stTrimAsciiSpaces(tag[2..]);
        }
        if (tag.len == etag.len) {
            let eq: bool = true;
            let k: usize = 0;
            while (k < tag.len) : (k += 1) {
                if (tag[k] != etag[k]) {
                    eq = false;
                    break;
                }
            }
            if (eq) return true;
        }
        if (end >= trimmed.len) break;
        pos = end + 1;
    }
    return false;
}

/// Read a regular file from disk into `out_buf` using POSIX `std.c.open`/`read`/`close`.
/// Returns byte count on success, or `null` if missing, unreadable, directory, or too large.
fn stReadRegularFileInto(full_path: string, out_buf: []u8): usize? {
    if (full_path.len == 0 || full_path.len >= STATIC_MAX_PATH_LEN) return null;

    let path_z_buf: [STATIC_MAX_PATH_LEN]u8 = [0; STATIC_MAX_PATH_LEN];
    let i: usize = 0;
    while (i < full_path.len) : (i += 1) {
        path_z_buf[i] = full_path[i];
    }
    path_z_buf[full_path.len] = 0;

    const path_z: [*:0]const u8 = path_z_buf[0..full_path.len :0].ptr;
    const z_flags = std.c.O{ .ACCMODE = .RDONLY, .CLOEXEC = true };
    const fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0));
    if (fd < 0) return null;

    let total: usize = 0;
    while (total < out_buf.len) {
        const n = std.c.read(fd, out_buf[total..out_buf.len].ptr, out_buf.len - total);
        if (n < 0) {
            * = std.c.close(fd);
            return null;
        }
        if (n == 0) break;
        total += (n as usize);
    }

    * = std.c.close(fd);
    return total;
}

fn stJoinFsPath(root_dir: string, rel_path: string, out_buf: []u8): usize? {
    let pos: usize = 0;
    let r_end: usize = root_dir.len;
    while (r_end > 1 && root_dir[r_end - 1] == '/') : (r_end -= 1) {}
    if (!stWriteRaw(out_buf, *pos, root_dir[0..r_end])) return null;
    if (rel_path.len > 0) {
        if (pos == 0 || out_buf[pos - 1] != '/') {
            if (!stWriteByte(out_buf, *pos, '/')) return null;
        }
        if (!stWriteRaw(out_buf, *pos, rel_path)) return null;
    }
    return pos;
}

fn stServeWithEtagBuf(
    cfg: StaticConfig,
    rel_path: string,
    if_none_match: string?,
    out_buf: []u8,
    etag_buf: []u8,
): Response {
    let norm_buf: [STATIC_MAX_PATH_LEN]u8 = [0; STATIC_MAX_PATH_LEN];
    const norm_len_opt = sanitizeRelPath(rel_path, norm_buf[0..]);
    if (norm_len_opt == null) {
        return Response.text(403, "Forbidden");
    }
    const norm_rel = norm_buf[0..norm_len_opt.?];

    let full_buf: [STATIC_MAX_PATH_LEN]u8 = [0; STATIC_MAX_PATH_LEN];
    let resolved_path_len: usize = 0;
    let read_len_opt: usize? = null;

    if (norm_rel.len == 0) {
        // Directory root -> serve index_file
        if (cfg.index_file.len > 0) {
            if (p_len := stJoinFsPath(cfg.root_dir, cfg.index_file, full_buf[0..])) {
                resolved_path_len = p_len;
                read_len_opt = stReadRegularFileInto(full_buf[0..p_len], out_buf);
            }
        }
    } else {
        if (p_len := stJoinFsPath(cfg.root_dir, norm_rel, full_buf[0..])) {
            resolved_path_len = p_len;
            read_len_opt = stReadRegularFileInto(full_buf[0..p_len], out_buf);
        }
        // If direct read failed (e.g. subdirectory), try `<rel>/<index_file>`
        if (read_len_opt == null && cfg.index_file.len > 0) {
            let sub_buf: [STATIC_MAX_PATH_LEN]u8 = [0; STATIC_MAX_PATH_LEN];
            let spos: usize = 0;
            if (stWriteRaw(sub_buf[0..], *spos, norm_rel) &&
                stWriteByte(sub_buf[0..], *spos, '/') &&
                stWriteRaw(sub_buf[0..], *spos, cfg.index_file))
            {
                if (p_len := stJoinFsPath(cfg.root_dir, sub_buf[0..spos], full_buf[0..])) {
                    resolved_path_len = p_len;
                    read_len_opt = stReadRegularFileInto(full_buf[0..p_len], out_buf);
                }
            }
        }
    }

    if (read_len_opt == null) {
        return Response.text(404, "Not Found");
    }

    const content = out_buf[0..read_len_opt.?];
    const mime = detectMimeType(full_buf[0..resolved_path_len], content);

    let etag_slice: string = "";
    if (cfg.enable_etag) {
        if (etag_len := computeStrongEtag(content, etag_buf)) {
            etag_slice = etag_buf[0..etag_len];
            if (inm := if_none_match) {
                if (etagMatches(inm, etag_slice)) {
                    let resp304 = Response.statusCode(304);
                    resp304.content_type = "";
                    * = resp304.setHeader("ETag", etag_slice);
                    if (cfg.cache_control.len > 0) {
                        * = resp304.setHeader("Cache-Control", cfg.cache_control);
                    }
                    return resp304;
                }
            }
        }
    }

    let resp = Response.bytes(200, content, mime);
    if (cfg.cache_control.len > 0) {
        * = resp.setHeader("Cache-Control", cfg.cache_control);
    }
    if (etag_slice.len > 0) {
        * = resp.setHeader("ETag", etag_slice);
    }
    return resp;
}

/// Pure static file serving helper for direct unit testing and custom handlers.
pub fn serveStaticRequest(
    cfg: StaticConfig,
    rel_path: string,
    if_none_match: string?,
    out_buf: []u8,
): Response {
    return stServeWithEtagBuf(cfg, rel_path, if_none_match, out_buf, g_static_etag_buf[0..]);
}

/// Read and serve a file directly from a root directory into `out_buf`.
pub fn serveFileFromDir(root_dir: string, rel_path: string, out_buf: []u8): Response {
    const cfg = StaticConfig{
        root_dir: root_dir,
        url_prefix: "",
        index_file: "index.html",
        cache_control: "public, max-age=3600",
        enable_etag: true,
    };
    return serveStaticRequest(cfg, rel_path, null, out_buf);
}

/// Serve a static file using `ctx.arenaAlloc` when a `RequestArena` is attached to `ctx`,
/// falling back to the internal static buffer otherwise.
pub fn serveStaticIntoArena(ctx: *Context, cfg: StaticConfig, rel_path: string): Response {
    const inm = ctx.header("If-None-Match");

    let file_buf: []u8 = g_static_file_buf[0..];
    if (arena_slice := ctx.arenaAlloc(STATIC_MAX_FILE_BYTES)) {
        file_buf = arena_slice;
    }

    let etag_buf: []u8 = g_static_etag_buf[0..];
    if (arena_etag := ctx.arenaAlloc(STATIC_ETAG_BUF_LEN)) {
        etag_buf = arena_etag;
    }

    return stServeWithEtagBuf(cfg, rel_path, inm, file_buf, etag_buf);
}

fn stRouteHandler(ctx: *Context): Response {
    let rel_path: string = "";
    if (w := ctx.param("wildcard")) {
        rel_path = w;
    } else if (w2 := ctx.param("*")) {
        rel_path = w2;
    }

    const inm = ctx.header("If-None-Match");
    return stServeWithEtagBuf(g_static_cfg, rel_path, inm, g_static_file_buf[0..], g_static_etag_buf[0..]);
}

/// Register the static file server plugin on `app` under `<cfg.url_prefix>/*`.
pub fn registerStatic(app: *App, cfg: StaticConfig): *App {
    g_static_cfg = cfg;

    let pat_buf: [256]u8 = [0; 256];
    let pos: usize = 0;

    let pref = cfg.url_prefix;
    while (pref.len > 0 && pref[pref.len - 1] == '/') {
        pref = pref[0 .. pref.len - 1];
    }

    if (pref.len == 0) {
        * = stWriteRaw(pat_buf[0..], *pos, "/*");
    } else {
        if (pref[0] != '/') {
            * = stWriteByte(pat_buf[0..], *pos, '/');
        }
        * = stWriteRaw(pat_buf[0..], *pos, pref);
        * = stWriteRaw(pat_buf[0..], *pos, "/*");
    }

    * = app.get(pat_buf[0..pos], &stRouteHandler);
    return app;
}
