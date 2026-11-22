// Source: Absorbed from /data/dev/bun/src/bundler/HTMLImportManifest.cp | Synced: 2026-11-22T01:40:00Z
// Pure-CP zero-allocation HTML Import Manifest builder & RFC 8259 JSON serializer
// (generates `{"index":...,"files":[{"input":...,"path":...,"loader":...,"isEntry":...,"headers":{"etag":...,"content-type":...}}]}`).

pub const MAX_HTML_MANIFEST_FILES: usize = 64;

/// Bundler loader kind recorded in an HTML import manifest entry.
pub enum HtmlManifestLoader: u8 {
    js = 0,
    css = 1,
    html = 2,
    json = 3,
    wasm = 4,
    file = 5,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .js => "js",
            .css => "css",
            .html => "html",
            .json => "json",
            .wasm => "wasm",
            .file => "file",
        };
    }

    /// Derive HTTP `content-type` header value from loader and output path extension.
    pub fn contentType(path: []const u8): []const u8 {
        return switch (_) {
            .js => "text/javascript;charset=utf-8",
            .css => "text/css;charset=utf-8",
            .html => "text/html;charset=utf-8",
            .json => "application/json;charset=utf-8",
            .wasm => "application/wasm",
            .file => mimeFromExt(path),
        };
    }
}

/// Single generated artifact entry inside an `HtmlImportManifest`.
pub struct HtmlManifestFile {
    input: []const u8 = "",
    path: []const u8 = "",
    loader: HtmlManifestLoader = .file,
    is_entry: bool = false,
    hash: u64 = 0,

    pub fn contentType(): []const u8 {
        return _.loader.contentType(_.path);
    }
}

const B64URL: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/// Encode a 64-bit little-endian content hash into an 11-char URL-safe Base64 ETag (no padding),
/// matching Bun's `HTMLImportManifest` ETag encoding.
pub fn encodeHashEtag(hash: u64, out: *[11]u8): []const u8 {
    var_le: {
        let b: [8]u8 = [0; 8];
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            const sh: u6 = @intCast(i * 8);
            b[i] = @as(u8, @truncate(hash >> sh));
        }
        // Group 0: b[0..3] -> 4 chars
        out.*[0] = B64URL[@as(usize, b[0] >> 2)];
        out.*[1] = B64URL[@as(usize, ((b[0] & 0x03) << 4) | (b[1] >> 4))];
        out.*[2] = B64URL[@as(usize, ((b[1] & 0x0F) << 2) | (b[2] >> 6))];
        out.*[3] = B64URL[@as(usize, b[2] & 0x3F)];
        // Group 1: b[3..6] -> 4 chars
        out.*[4] = B64URL[@as(usize, b[3] >> 2)];
        out.*[5] = B64URL[@as(usize, ((b[3] & 0x03) << 4) | (b[4] >> 4))];
        out.*[6] = B64URL[@as(usize, ((b[4] & 0x0F) << 2) | (b[5] >> 6))];
        out.*[7] = B64URL[@as(usize, b[5] & 0x3F)];
        // Group 2: b[6..8] -> 3 chars (no padding)
        out.*[8] = B64URL[@as(usize, b[6] >> 2)];
        out.*[9] = B64URL[@as(usize, ((b[6] & 0x03) << 4) | (b[7] >> 4))];
        out.*[10] = B64URL[@as(usize, (b[7] & 0x0F) << 2)];
        break :var_le;
    }
    return out.*[0..11];
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) : (i += 1) {
        if (asciiLower(s[off + i]) != asciiLower(suffix[i])) return false;
    }
    return true;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn mimeFromExt(path: []const u8): []const u8 {
    if (endsWithIgnoreCase(path, ".svg")) return "image/svg+xml";
    if (endsWithIgnoreCase(path, ".png")) return "image/png";
    if (endsWithIgnoreCase(path, ".jpg") || endsWithIgnoreCase(path, ".jpeg")) return "image/jpeg";
    if (endsWithIgnoreCase(path, ".webp")) return "image/webp";
    if (endsWithIgnoreCase(path, ".gif")) return "image/gif";
    if (endsWithIgnoreCase(path, ".ico")) return "image/x-icon";
    if (endsWithIgnoreCase(path, ".woff2")) return "font/woff2";
    if (endsWithIgnoreCase(path, ".woff")) return "font/woff";
    if (endsWithIgnoreCase(path, ".ttf")) return "font/ttf";
    if (endsWithIgnoreCase(path, ".txt")) return "text/plain;charset=utf-8";
    return "application/octet-stream";
}

fn appendBytes(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn appendJsonString(out: []u8, pos: *usize, s: []const u8): bool {
    if (!appendBytes(out, pos, "\"")) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"') {
            if (!appendBytes(out, pos, "\\\"")) return false;
        } else if (c == '\\') {
            if (!appendBytes(out, pos, "\\\\")) return false;
        } else if (c == '\n') {
            if (!appendBytes(out, pos, "\\n")) return false;
        } else if (c == '\r') {
            if (!appendBytes(out, pos, "\\r")) return false;
        } else if (c == '\t') {
            if (!appendBytes(out, pos, "\\t")) return false;
        } else if (c < 0x20) {
            if (!appendBytes(out, pos, "\\u00")) return false;
            const hex: []const u8 = "0123456789abcdef";
            if (pos.* + 2 > out.len) return false;
            out[pos.*] = hex[@as(usize, (c >> 4) & 0xF)];
            out[pos.* + 1] = hex[@as(usize, c & 0xF)];
            pos.* += 2;
        } else {
            if (pos.* >= out.len) return false;
            out[pos.*] = c;
            pos.* += 1;
        }
    }
    return appendBytes(out, pos, "\"");
}

/// Zero-allocation HTML Import Manifest builder & JSON serializer.
pub struct HtmlImportManifest {
    index_path: []const u8 = "",
    files: [MAX_HTML_MANIFEST_FILES]HtmlManifestFile = [HtmlManifestFile{}; MAX_HTML_MANIFEST_FILES],
    file_count: usize = 0,

    pub static fn init(index_path: []const u8): HtmlImportManifest {
        return HtmlImportManifest{
            index_path: index_path,
        };
    }

    pub fn count(): usize {
        return _.file_count;
    }

    /// Add a generated bundle artifact to the manifest (deduplicates by `path`).
    pub fn addFile(*_, entry: HtmlManifestFile): bool {
        if (entry.path.len == 0) return false;
        let i: usize = 0;
        while (i < _.file_count) : (i += 1) {
            if (strEq(_.files[i].path, entry.path)) {
                _.files[i] = entry;
                return true;
            }
        }
        if (_.file_count >= MAX_HTML_MANIFEST_FILES) return false;
        _.files[_.file_count] = entry;
        _.file_count += 1;
        return true;
    }

    /// Look up a generated artifact entry by output `path` (for HTTP static serving & ETag checks).
    pub fn findByPath(path: []const u8): HtmlManifestFile? {
        if (path.len == 0) return null;
        let i: usize = 0;
        while (i < _.file_count) : (i += 1) {
            if (strEq(_.files[i].path, path)) {
                return _.files[i];
            }
        }
        return null;
    }

    /// Serialize the manifest into Bun-compatible JSON inside `out` (returns `null` on overflow).
    pub fn toJson(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (!appendBytes(out, &pos, "{\"index\":")) return null;
        if (!appendJsonString(out, &pos, _.index_path)) return null;
        if (!appendBytes(out, &pos, ",\"files\":[")) return null;

        let i: usize = 0;
        while (i < _.file_count) : (i += 1) {
            if (i > 0) {
                if (!appendBytes(out, &pos, ",")) return null;
            }
            const f = _.files[i];
            if (!appendBytes(out, &pos, "{")) return null;
            if (f.input.len > 0) {
                if (!appendBytes(out, &pos, "\"input\":")) return null;
                if (!appendJsonString(out, &pos, f.input)) return null;
                if (!appendBytes(out, &pos, ",")) return null;
            }
            if (!appendBytes(out, &pos, "\"path\":")) return null;
            if (!appendJsonString(out, &pos, f.path)) return null;
            if (!appendBytes(out, &pos, ",\"loader\":\"")) return null;
            if (!appendBytes(out, &pos, f.loader.asStr())) return null;
            if (!appendBytes(out, &pos, "\",\"isEntry\":")) return null;
            if (!appendBytes(out, &pos, if (f.is_entry) "true" else "false")) return null;
            if (!appendBytes(out, &pos, ",\"headers\":{")) return null;

            if (f.hash > 0) {
                let etag_buf: [11]u8 = [0; 11];
                const etag = encodeHashEtag(f.hash, &etag_buf);
                if (!appendBytes(out, &pos, "\"etag\":\"")) return null;
                if (!appendBytes(out, &pos, etag)) return null;
                if (!appendBytes(out, &pos, "\",")) return null;
            }

            if (!appendBytes(out, &pos, "\"content-type\":\"")) return null;
            if (!appendBytes(out, &pos, f.contentType())) return null;
            if (!appendBytes(out, &pos, "\"}}")) return null;
        }

        if (!appendBytes(out, &pos, "]}")) return null;
        return out[0..pos];
    }
}
