// Official @web/multipart Plugin for `cap:web`
// Zero-heap multipart/form-data parser & request size guard (Fastify/Axum inspired).

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub const MP_MAX_PARTS: usize = 16;

pub struct MultipartConfig {
    max_parts: usize = 16;
    max_file_bytes: usize = 1048576;
    max_total_bytes: usize = 4194304;
    allowed_mime_prefix: string = "";
}

pub struct MultipartPart {
    name: string = "";
    filename: string = "";
    content_type: string = "";
    data: string = "";
    is_file: bool = false;
}

pub struct MultipartForm {
    parts: [16]MultipartPart = [MultipartPart{}; 16];
    count: usize = 0;
    total_bytes: usize = 0;
}

let g_mp_cfg: MultipartConfig = MultipartConfig{};

fn mpAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn mpStartsWithIgnoreCase(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (mpAsciiLower(s[i]) != mpAsciiLower(prefix[i])) return false;
    }
    return true;
}

fn mpEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (mpAsciiLower(a[i]) != mpAsciiLower(b[i])) return false;
    }
    return true;
}

fn mpTrimSpaces(s: string): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) : (end -= 1) {}
    return s[start..end];
}

fn mpParseUsize(s: string): usize? {
    const trimmed = mpTrimSpaces(s);
    if (trimmed.len == 0) return null;
    let acc: usize = 0;
    let i: usize = 0;
    while (i < trimmed.len) : (i += 1) {
        const c = trimmed[i];
        if (c < '0' || c > '9') return null;
        acc = acc * 10 + ((c - '0') as usize);
    }
    return acc;
}

fn mpMatchAt(haystack: string, pos: usize, needle: string): bool {
    if (pos + needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i < needle.len) : (i += 1) {
        if (haystack[pos + i] != needle[i]) return false;
    }
    return true;
}

/// Find `--<boundary>` at or after `start_pos`. Returns index of the first `-`.
fn mpFindBoundary(body: string, start_pos: usize, boundary: string): usize? {
    const delim_len = boundary.len + 2;
    if (delim_len > body.len) return null;
    let i: usize = start_pos;
    while (i + delim_len <= body.len) : (i += 1) {
        if (body[i] == '-' && body[i + 1] == '-' && mpMatchAt(body, i + 2, boundary)) {
            return i;
        }
    }
    return null;
}

/// Find `\r\n\r\n` or `\n\n` separating headers from part body.
/// Returns header end index; sets `body_start_out` to the first byte after separator.
fn mpFindHeaderEnd(part: string, body_start_out: *usize): usize? {
    let i: usize = 0;
    while (i < part.len) : (i += 1) {
        if (i + 3 < part.len && part[i] == '\r' && part[i + 1] == '\n' && part[i + 2] == '\r' && part[i + 3] == '\n') {
            body_start_out.* = i + 4;
            return i;
        }
        if (i + 1 < part.len && part[i] == '\n' && part[i + 1] == '\n') {
            body_start_out.* = i + 2;
            return i;
        }
    }
    return null;
}

/// Extract parameter `key="..."` or `key=value` from a `Content-Disposition` value.
fn mpExtractParam(header_val: string, key: string): string? {
    let pos: usize = 0;
    while (pos < header_val.len) {
        let end: usize = pos;
        while (end < header_val.len && header_val[end] != ';') : (end += 1) {}
        const seg = mpTrimSpaces(header_val[pos..end]);
        let eq: usize = 0;
        while (eq < seg.len && seg[eq] != '=') : (eq += 1) {}
        if (eq < seg.len) {
            const k = mpTrimSpaces(seg[0..eq]);
            if (mpEqlIgnoreCase(k, key)) {
                let v = mpTrimSpaces(seg[eq + 1 ..]);
                if (v.len >= 2 && v[0] == '"' && v[v.len - 1] == '"') {
                    v = v[1 .. v.len - 1];
                }
                return v;
            }
        }
        if (end >= header_val.len) break;
        pos = end + 1;
    }
    return null;
}

/// Parse an RFC 7578 `multipart/form-data` payload into `out_form`.
/// Returns:
///   - `0` on success
///   - `400` on malformed multipart structure or missing `name`
///   - `413` on exceeding `max_parts`, `max_file_bytes`, or `max_total_bytes`
///   - `415` on disallowed MIME prefix when `cfg.allowed_mime_prefix` is set
pub fn parseMultipartBody(
    boundary: string,
    body: string,
    cfg: MultipartConfig,
    out_form: *MultipartForm,
): i32 {
    out_form.count = 0;
    out_form.total_bytes = 0;

    if (boundary.len == 0) return 400;
    if (body.len > cfg.max_total_bytes) return 413;

    const first = mpFindBoundary(body, 0, boundary) ?? return 400;
    let cursor: usize = first + 2 + boundary.len;

    // Empty multipart: `--boundary--`
    if (cursor + 1 < body.len && body[cursor] == '-' && body[cursor + 1] == '-') {
        return 0;
    }
    if (cursor + 1 < body.len && body[cursor] == '\r' && body[cursor + 1] == '\n') {
        cursor += 2;
    } else if (cursor < body.len && body[cursor] == '\n') {
        cursor += 1;
    } else {
        return 400;
    }

    const eff_max_parts: usize = if (cfg.max_parts < MP_MAX_PARTS) cfg.max_parts else MP_MAX_PARTS;

    while (cursor < body.len) {
        const next_delim = mpFindBoundary(body, cursor, boundary) ?? return 400;

        // Strip trailing CRLF or LF immediately preceding the boundary line
        let part_end: usize = next_delim;
        if (part_end >= cursor + 2 && body[part_end - 2] == '\r' && body[part_end - 1] == '\n') {
            part_end -= 2;
        } else if (part_end >= cursor + 1 && body[part_end - 1] == '\n') {
            part_end -= 1;
        }

        const part_slice = body[cursor..part_end];
        let data_start: usize = 0;
        const hdr_end = mpFindHeaderEnd(part_slice, *data_start) ?? return 400;
        const hdr_block = part_slice[0..hdr_end];
        const part_data = part_slice[data_start..];

        let p_name: string = "";
        let p_filename: string = "";
        let p_ctype: string = "";
        let has_name: bool = false;

        let line_pos: usize = 0;
        while (line_pos < hdr_block.len) {
            let line_end: usize = line_pos;
            while (line_end < hdr_block.len && hdr_block[line_end] != '\n') : (line_end += 1) {}
            const raw_line = mpTrimSpaces(hdr_block[line_pos..line_end]);
            if (raw_line.len > 0) {
                let colon: usize = 0;
                while (colon < raw_line.len && raw_line[colon] != ':') : (colon += 1) {}
                if (colon < raw_line.len) {
                    const h_name = mpTrimSpaces(raw_line[0..colon]);
                    const h_val = mpTrimSpaces(raw_line[colon + 1 ..]);
                    if (mpEqlIgnoreCase(h_name, "Content-Disposition")) {
                        if (n_val := mpExtractParam(h_val, "name")) {
                            p_name = n_val;
                            has_name = true;
                        }
                        if (f_val := mpExtractParam(h_val, "filename")) {
                            p_filename = f_val;
                        }
                    } else if (mpEqlIgnoreCase(h_name, "Content-Type")) {
                        p_ctype = h_val;
                    }
                }
            }
            if (line_end >= hdr_block.len) break;
            line_pos = line_end + 1;
        }

        if (!has_name || p_name.len == 0) return 400;

        const is_file: bool = p_filename.len > 0;
        if (out_form.count >= eff_max_parts) return 413;
        if (is_file && part_data.len > cfg.max_file_bytes) return 413;
        if (out_form.total_bytes + part_data.len > cfg.max_total_bytes) return 413;

        if (cfg.allowed_mime_prefix.len > 0 && (is_file || p_ctype.len > 0)) {
            if (!mpStartsWithIgnoreCase(p_ctype, cfg.allowed_mime_prefix)) {
                return 415;
            }
        }

        out_form.parts[out_form.count] = MultipartPart{
            name: p_name,
            filename: p_filename,
            content_type: p_ctype,
            data: part_data,
            is_file: is_file,
        };
        out_form.count += 1;
        out_form.total_bytes += part_data.len;

        cursor = next_delim + 2 + boundary.len;
        if (cursor + 1 < body.len && body[cursor] == '-' && body[cursor + 1] == '-') {
            return 0;
        }
        if (cursor + 1 < body.len && body[cursor] == '\r' && body[cursor + 1] == '\n') {
            cursor += 2;
        } else if (cursor < body.len && body[cursor] == '\n') {
            cursor += 1;
        } else {
            return 400;
        }
    }

    return 400;
}

/// Lifecycle guard hook: validates `max_total_bytes` for `multipart/form-data` requests.
/// Returns `413 Payload Too Large` when the body or `Content-Length` exceeds the configured limit.
pub fn multipartGuardHook(ctx: *Context): bool {
    const ct = ctx.header("Content-Type") ?? return true;
    if (!mpStartsWithIgnoreCase(ct, "multipart/form-data")) return true;

    const max_bytes = g_mp_cfg.max_total_bytes;
    if (ctx.body.len > max_bytes) {
        ctx.abort(Response.json(413, "{\"error\":\"Payload Too Large\",\"message\":\"Multipart body exceeds max_total_bytes\"}"));
        return false;
    }

    if (cl_hdr := ctx.header("Content-Length")) {
        if (cl_val := mpParseUsize(cl_hdr)) {
            if (cl_val > max_bytes) {
                ctx.abort(Response.json(413, "{\"error\":\"Payload Too Large\",\"message\":\"Content-Length exceeds max_total_bytes\"}"));
                return false;
            }
        }
    }

    return true;
}

/// Build a reusable `cap:web` `Plugin` for `@web/multipart`.
pub fn createMultipartPlugin(cfg: MultipartConfig): Plugin {
    g_mp_cfg = cfg;
    let p = Plugin.init("@web/multipart");
    * = p.withScope(PluginScope.Global).onBeforeHandle(multipartGuardHook);
    return p;
}

/// One-call registration onto a `cap:web` `App`.
pub fn registerMultipart(app: *App, cfg: MultipartConfig): *App {
    const p = createMultipartPlugin(cfg);
    return app.usePlugin(&p);
}
