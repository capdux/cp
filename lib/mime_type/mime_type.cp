// Source: absorbed from bun/src/http_types/MimeType.cp (cap:mime_type)
const std = @import("std");

pub const Category = enum {
    none,
    image,
    text,
    html,
    font,
    other,
    css,
    json,
    audio,
    video,
    javascript,
    wasm,
    application,
    model,
    message,
    @"x-conference",
    @"x-shader",
    chemical,
    multipart,

    pub fn isCode(self: Category): bool {
        return switch (self) {
            .wasm, .json, .css, .html, .javascript => true,
            else => false,
        };
    }

    pub fn isTextLike(self: Category): bool {
        return switch (self) {
            .javascript, .html, .text, .css, .json => true,
            else => false,
        };
    }

    pub fn autosetFilename(self: Category): bool {
        return switch (self) {
            .wasm, .font, .image, .audio, .video, .javascript, .html, .text, .css, .json => false,
            else => true,
        };
    }
};

pub fn classifyMime(mime_str: []const u8): Category {
    const slash = std.mem.indexOfScalar(u8, mime_str, '/') orelse return .none;
    const cat = mime_str[0..slash];
    let sub = if (mime_str.len > slash + 1) mime_str[slash + 1 ..] else "";
    if (semi := std.mem.indexOfScalar(u8, sub, ';')) {
        sub = std.mem.trimRight(u8, sub[0..semi], " \t");
    }

    if (std.ascii.eqlIgnoreCase(cat, "text")) {
        if (std.ascii.eqlIgnoreCase(sub, "javascript")) { return .javascript; }
        if (std.ascii.eqlIgnoreCase(sub, "css")) { return .css; }
        if (std.ascii.eqlIgnoreCase(sub, "html")) { return .html; }
        if (std.ascii.eqlIgnoreCase(sub, "json")) { return .json; }
        return .text;
    }
    if (std.ascii.eqlIgnoreCase(cat, "application")) {
        if (std.ascii.eqlIgnoreCase(sub, "wasm")) { return .wasm; }
        if (std.ascii.eqlIgnoreCase(sub, "javascript")) { return .javascript; }
        if (std.ascii.eqlIgnoreCase(sub, "json")) { return .json; }
        if (std.ascii.eqlIgnoreCase(sub, "octet-stream")) { return .other; }
        return .application;
    }
    if (std.ascii.eqlIgnoreCase(cat, "image")) { return .image; }
    if (std.ascii.eqlIgnoreCase(cat, "video")) { return .video; }
    if (std.ascii.eqlIgnoreCase(cat, "audio")) { return .audio; }
    if (std.ascii.eqlIgnoreCase(cat, "font")) { return .font; }
    if (std.ascii.eqlIgnoreCase(cat, "multipart")) { return .multipart; }
    if (std.ascii.eqlIgnoreCase(cat, "model")) { return .model; }
    if (std.ascii.eqlIgnoreCase(cat, "message")) { return .message; }
    if (std.ascii.eqlIgnoreCase(cat, "chemical")) { return .chemical; }
    return .other;
}

const ext_map = std.StaticStringMap([]const u8).initComptime(.{
    .{ "html", "text/html; charset=utf-8" },
    .{ "htm", "text/html; charset=utf-8" },
    .{ "css", "text/css; charset=utf-8" },
    .{ "js", "text/javascript; charset=utf-8" },
    .{ "mjs", "text/javascript; charset=utf-8" },
    .{ "cjs", "text/javascript; charset=utf-8" },
    .{ "ts", "text/typescript; charset=utf-8" },
    .{ "tsx", "text/tsx; charset=utf-8" },
    .{ "jsx", "text/jsx; charset=utf-8" },
    .{ "json", "application/json; charset=utf-8" },
    .{ "map", "application/json; charset=utf-8" },
    .{ "wasm", "application/wasm" },
    .{ "txt", "text/plain; charset=utf-8" },
    .{ "md", "text/markdown; charset=utf-8" },
    .{ "csv", "text/csv; charset=utf-8" },
    .{ "xml", "application/xml" },
    .{ "yaml", "application/yaml" },
    .{ "yml", "application/yaml" },
    .{ "toml", "application/toml" },
    .{ "svg", "image/svg+xml" },
    .{ "png", "image/png" },
    .{ "jpg", "image/jpeg" },
    .{ "jpeg", "image/jpeg" },
    .{ "gif", "image/gif" },
    .{ "webp", "image/webp" },
    .{ "avif", "image/avif" },
    .{ "ico", "image/x-icon" },
    .{ "woff", "font/woff" },
    .{ "woff2", "font/woff2" },
    .{ "ttf", "font/ttf" },
    .{ "otf", "font/otf" },
    .{ "mp3", "audio/mpeg" },
    .{ "wav", "audio/wav" },
    .{ "ogg", "audio/ogg" },
    .{ "mp4", "video/mp4" },
    .{ "webm", "video/webm" },
    .{ "pdf", "application/pdf" },
    .{ "zip", "application/zip" },
    .{ "gz", "application/gzip" },
    .{ "tar", "application/x-tar" },
    .{ "zst", "application/zstd" },
    .{ "bin", "application/octet-stream" },
});

pub fn byExtension(ext: []const u8): ([]const u8)? {
    const clean = if (ext.len > 0 and ext[0] == '.') ext[1..] else ext;
    if (clean.len == 0 or clean.len > 16) { return null; }
    let lower: [16]u8 = undefined;
    let i: usize = 0;
    while (i < clean.len) : (i += 1) {
        lower[i] = std.ascii.toLower(clean[i]);
    }
    return ext_map.get(lower[0..clean.len]);
}

pub fn byPath(path: []const u8): ([]const u8)? {
    const dot = std.mem.lastIndexOfScalar(u8, path, '.') orelse return null;
    if (slash := std.mem.lastIndexOfScalar(u8, path, '/')) {
        if (slash > dot) { return null; }
    }
    return byExtension(path[dot + 1 ..]);
}
