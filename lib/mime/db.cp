// Source: cp/lib/mime/db.cp (Comprehensive Extension <-> MIME Database & Classification)
// Pure CP zero-C-dependency MIME database covering 110+ Web, Font, Media, Archive,
// Document, Source Code, and AI/ML data formats.

const std = @import("std");

struct MimeEntry {
    ext: []const u8;
    mime: []const u8;
    is_primary: bool = true;
}

fn getMimeEntries(): []const MimeEntry {
    const entries: []const MimeEntry = [
    // Web & Markup
    MimeEntry{ ext: "html", mime: "text/html", is_primary: true },
    MimeEntry{ ext: "htm", mime: "text/html", is_primary: false },
    MimeEntry{ ext: "css", mime: "text/css", is_primary: true },
    MimeEntry{ ext: "js", mime: "text/javascript", is_primary: true },
    MimeEntry{ ext: "mjs", mime: "text/javascript", is_primary: false },
    MimeEntry{ ext: "cjs", mime: "application/javascript", is_primary: false },
    MimeEntry{ ext: "ts", mime: "application/typescript", is_primary: true },
    MimeEntry{ ext: "mts", mime: "application/typescript", is_primary: false },
    MimeEntry{ ext: "cts", mime: "application/typescript", is_primary: false },
    MimeEntry{ ext: "jsx", mime: "text/jsx", is_primary: true },
    MimeEntry{ ext: "tsx", mime: "text/tsx", is_primary: true },
    MimeEntry{ ext: "wasm", mime: "application/wasm", is_primary: true },
    MimeEntry{ ext: "json", mime: "application/json", is_primary: true },
    MimeEntry{ ext: "map", mime: "application/json", is_primary: false },
    MimeEntry{ ext: "jsonld", mime: "application/ld+json", is_primary: true },
    MimeEntry{ ext: "webmanifest", mime: "application/manifest+json", is_primary: true },
    MimeEntry{ ext: "xml", mime: "application/xml", is_primary: true },
    MimeEntry{ ext: "xhtml", mime: "application/xhtml+xml", is_primary: true },
    MimeEntry{ ext: "rss", mime: "application/rss+xml", is_primary: true },
    MimeEntry{ ext: "atom", mime: "application/atom+xml", is_primary: true },
    MimeEntry{ ext: "svg", mime: "image/svg+xml", is_primary: true },
    MimeEntry{ ext: "ics", mime: "text/calendar", is_primary: true },
    MimeEntry{ ext: "vcf", mime: "text/vcard", is_primary: true },

    // Text, Config & Documentation
    MimeEntry{ ext: "txt", mime: "text/plain", is_primary: true },
    MimeEntry{ ext: "log", mime: "text/plain", is_primary: false },
    MimeEntry{ ext: "conf", mime: "text/plain", is_primary: false },
    MimeEntry{ ext: "env", mime: "text/plain", is_primary: false },
    MimeEntry{ ext: "md", mime: "text/markdown", is_primary: true },
    MimeEntry{ ext: "markdown", mime: "text/markdown", is_primary: false },
    MimeEntry{ ext: "mdx", mime: "text/mdx", is_primary: true },
    MimeEntry{ ext: "csv", mime: "text/csv", is_primary: true },
    MimeEntry{ ext: "tsv", mime: "text/tab-separated-values", is_primary: true },
    MimeEntry{ ext: "yaml", mime: "application/yaml", is_primary: true },
    MimeEntry{ ext: "yml", mime: "application/yaml", is_primary: false },
    MimeEntry{ ext: "toml", mime: "application/toml", is_primary: true },
    MimeEntry{ ext: "ini", mime: "text/plain", is_primary: false },
    MimeEntry{ ext: "rtf", mime: "application/rtf", is_primary: true },

    // Source Code & Protocols
    MimeEntry{ ext: "cp", mime: "text/x-cp", is_primary: true },
    MimeEntry{ ext: "zig", mime: "text/x-zig", is_primary: true },
    MimeEntry{ ext: "rs", mime: "text/x-rust", is_primary: true },
    MimeEntry{ ext: "c", mime: "text/x-c", is_primary: true },
    MimeEntry{ ext: "h", mime: "text/x-c", is_primary: false },
    MimeEntry{ ext: "cpp", mime: "text/x-c++src", is_primary: true },
    MimeEntry{ ext: "cc", mime: "text/x-c++src", is_primary: false },
    MimeEntry{ ext: "hpp", mime: "text/x-c++hdr", is_primary: true },
    MimeEntry{ ext: "py", mime: "text/x-python", is_primary: true },
    MimeEntry{ ext: "go", mime: "text/x-go", is_primary: true },
    MimeEntry{ ext: "java", mime: "text/x-java-source", is_primary: true },
    MimeEntry{ ext: "sh", mime: "application/x-sh", is_primary: true },
    MimeEntry{ ext: "sql", mime: "application/sql", is_primary: true },
    MimeEntry{ ext: "graphql", mime: "application/graphql", is_primary: true },
    MimeEntry{ ext: "gql", mime: "application/graphql", is_primary: false },
    MimeEntry{ ext: "proto", mime: "text/x-protobuf", is_primary: true },

    // AI / ML / Data Science Formats
    MimeEntry{ ext: "jsonl", mime: "application/jsonl", is_primary: true },
    MimeEntry{ ext: "ndjson", mime: "application/x-ndjson", is_primary: true },
    MimeEntry{ ext: "parquet", mime: "application/vnd.apache.parquet", is_primary: true },
    MimeEntry{ ext: "arrow", mime: "application/vnd.apache.arrow.file", is_primary: true },
    MimeEntry{ ext: "onnx", mime: "application/x-onnx", is_primary: true },
    MimeEntry{ ext: "safetensors", mime: "application/x-safetensors", is_primary: true },
    MimeEntry{ ext: "gguf", mime: "application/x-gguf", is_primary: true },
    MimeEntry{ ext: "npy", mime: "application/x-npy", is_primary: true },
    MimeEntry{ ext: "npz", mime: "application/x-npz", is_primary: true },
    MimeEntry{ ext: "cbor", mime: "application/cbor", is_primary: true },
    MimeEntry{ ext: "msgpack", mime: "application/msgpack", is_primary: true },

    // Fonts
    MimeEntry{ ext: "woff", mime: "font/woff", is_primary: true },
    MimeEntry{ ext: "woff2", mime: "font/woff2", is_primary: true },
    MimeEntry{ ext: "ttf", mime: "font/ttf", is_primary: true },
    MimeEntry{ ext: "otf", mime: "font/otf", is_primary: true },
    MimeEntry{ ext: "eot", mime: "application/vnd.ms-fontobject", is_primary: true },

    // Images
    MimeEntry{ ext: "png", mime: "image/png", is_primary: true },
    MimeEntry{ ext: "jpg", mime: "image/jpeg", is_primary: true },
    MimeEntry{ ext: "jpeg", mime: "image/jpeg", is_primary: false },
    MimeEntry{ ext: "gif", mime: "image/gif", is_primary: true },
    MimeEntry{ ext: "webp", mime: "image/webp", is_primary: true },
    MimeEntry{ ext: "avif", mime: "image/avif", is_primary: true },
    MimeEntry{ ext: "ico", mime: "image/x-icon", is_primary: true },
    MimeEntry{ ext: "cur", mime: "image/x-icon", is_primary: false },
    MimeEntry{ ext: "bmp", mime: "image/bmp", is_primary: true },
    MimeEntry{ ext: "tiff", mime: "image/tiff", is_primary: true },
    MimeEntry{ ext: "tif", mime: "image/tiff", is_primary: false },
    MimeEntry{ ext: "heic", mime: "image/heic", is_primary: true },
    MimeEntry{ ext: "heif", mime: "image/heif", is_primary: true },
    MimeEntry{ ext: "jxl", mime: "image/jxl", is_primary: true },

    // Audio
    MimeEntry{ ext: "mp3", mime: "audio/mpeg", is_primary: true },
    MimeEntry{ ext: "wav", mime: "audio/wav", is_primary: true },
    MimeEntry{ ext: "ogg", mime: "audio/ogg", is_primary: true },
    MimeEntry{ ext: "oga", mime: "audio/ogg", is_primary: false },
    MimeEntry{ ext: "opus", mime: "audio/opus", is_primary: true },
    MimeEntry{ ext: "flac", mime: "audio/flac", is_primary: true },
    MimeEntry{ ext: "aac", mime: "audio/aac", is_primary: true },
    MimeEntry{ ext: "m4a", mime: "audio/mp4", is_primary: true },
    MimeEntry{ ext: "weba", mime: "audio/webm", is_primary: true },
    MimeEntry{ ext: "mid", mime: "audio/midi", is_primary: true },
    MimeEntry{ ext: "midi", mime: "audio/midi", is_primary: false },

    // Video
    MimeEntry{ ext: "mp4", mime: "video/mp4", is_primary: true },
    MimeEntry{ ext: "m4v", mime: "video/mp4", is_primary: false },
    MimeEntry{ ext: "webm", mime: "video/webm", is_primary: true },
    MimeEntry{ ext: "ogv", mime: "video/ogg", is_primary: true },
    MimeEntry{ ext: "mov", mime: "video/quicktime", is_primary: true },
    MimeEntry{ ext: "avi", mime: "video/x-msvideo", is_primary: true },
    MimeEntry{ ext: "mkv", mime: "video/x-matroska", is_primary: true },
    MimeEntry{ ext: "mpeg", mime: "video/mpeg", is_primary: true },
    MimeEntry{ ext: "mpg", mime: "video/mpeg", is_primary: false },
    MimeEntry{ ext: "ts_vid", mime: "video/mp2t", is_primary: true },

    // Archives & Packages
    MimeEntry{ ext: "tar.gz", mime: "application/gzip", is_primary: false },
    MimeEntry{ ext: "tgz", mime: "application/gzip", is_primary: false },
    MimeEntry{ ext: "gz", mime: "application/gzip", is_primary: true },
    MimeEntry{ ext: "zip", mime: "application/zip", is_primary: true },
    MimeEntry{ ext: "tar", mime: "application/x-tar", is_primary: true },
    MimeEntry{ ext: "bz2", mime: "application/x-bzip2", is_primary: true },
    MimeEntry{ ext: "xz", mime: "application/x-xz", is_primary: true },
    MimeEntry{ ext: "zst", mime: "application/zstd", is_primary: true },
    MimeEntry{ ext: "7z", mime: "application/x-7z-compressed", is_primary: true },
    MimeEntry{ ext: "rar", mime: "application/vnd.rar", is_primary: true },
    MimeEntry{ ext: "apk", mime: "application/vnd.android.package-archive", is_primary: true },
    MimeEntry{ ext: "deb", mime: "application/vnd.debian.binary-package", is_primary: true },
    MimeEntry{ ext: "rpm", mime: "application/x-rpm", is_primary: true },

    // Office & Documents
    MimeEntry{ ext: "pdf", mime: "application/pdf", is_primary: true },
    MimeEntry{ ext: "doc", mime: "application/msword", is_primary: true },
    MimeEntry{ ext: "docx", mime: "application/vnd.openxmlformats-officedocument.wordprocessingml.document", is_primary: true },
    MimeEntry{ ext: "xls", mime: "application/vnd.ms-excel", is_primary: true },
    MimeEntry{ ext: "xlsx", mime: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", is_primary: true },
    MimeEntry{ ext: "ppt", mime: "application/vnd.ms-powerpoint", is_primary: true },
    MimeEntry{ ext: "pptx", mime: "application/vnd.openxmlformats-officedocument.presentationml.presentation", is_primary: true },
    MimeEntry{ ext: "odt", mime: "application/vnd.oasis.opendocument.text", is_primary: true },
    MimeEntry{ ext: "epub", mime: "application/epub+zip", is_primary: true },

    // Binary / Generic / Certificates
    MimeEntry{ ext: "bin", mime: "application/octet-stream", is_primary: true },
    MimeEntry{ ext: "exe", mime: "application/octet-stream", is_primary: false },
    MimeEntry{ ext: "dll", mime: "application/octet-stream", is_primary: false },
    MimeEntry{ ext: "so", mime: "application/octet-stream", is_primary: false },
    MimeEntry{ ext: "dylib", mime: "application/octet-stream", is_primary: false },
    MimeEntry{ ext: "pem", mime: "application/x-pem-file", is_primary: true },
    MimeEntry{ ext: "crt", mime: "application/x-x509-ca-cert", is_primary: true },
    MimeEntry{ ext: "der", mime: "application/x-x509-ca-cert", is_primary: false },
    MimeEntry{ ext: "p12", mime: "application/x-pkcs12", is_primary: true },
    MimeEntry{ ext: "sqlite", mime: "application/vnd.sqlite3", is_primary: true },
    MimeEntry{ ext: "db", mime: "application/vnd.sqlite3", is_primary: false },
];
    return entries;
}

fn asciiLowerChar(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (asciiLowerChar(a[i]) != asciiLowerChar(b[i])) return false;
    }
    return true;
}

fn asciiStartsWithIgnoreCase(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    return asciiEqlIgnoreCase(haystack[0..prefix.len], prefix);
}

fn asciiEndsWithIgnoreCase(haystack: []const u8, suffix: []const u8): bool {
    if (haystack.len < suffix.len) return false;
    return asciiEqlIgnoreCase(haystack[(haystack.len - suffix.len)..], suffix);
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[start..end];
}

fn extractEssence(mime_type: []const u8): []const u8 {
    let i: usize = 0;
    while (i < mime_type.len) : (i += 1) {
        if (mime_type[i] == ';') {
            return trimOws(mime_type[0..i]);
        }
    }
    return trimOws(mime_type);
}

fn lookupExactExt(ext: []const u8): ([]const u8)? {
    if (ext.len == 0) return null;
    for (entry of getMimeEntries()) {
        if (asciiEqlIgnoreCase(entry.ext, ext)) {
            return entry.mime;
        }
    }
    return null;
}

/// Comprehensive bidirectional MIME <-> extension database and media classification helpers.
pub static class MimeDb {
    /// Looks up a MIME type from a file path, filename, or extension (case-insensitive).
    /// Strips directory components and leading dots; supports compound extensions such as "tar.gz".
    pub fn fromExtension(ext_or_path: []const u8): ([]const u8)? {
        const trimmed = trimOws(ext_or_path);
        if (trimmed.len == 0) return null;

        // Strip directory path ('/' or '\\')
        let base_start: usize = 0;
        let has_sep = false;
        let i: usize = 0;
        while (i < trimmed.len) : (i += 1) {
            if (trimmed[i] == '/' || trimmed[i] == '\\') {
                base_start = i + 1;
                has_sep = true;
            }
        }
        const base = trimmed[base_start..];
        if (base.len == 0) return null;

        // Find first '.' and last '.' in basename
        let first_dot: usize? = null;
        let last_dot: usize? = null;
        let j: usize = 0;
        while (j < base.len) : (j += 1) {
            if (base[j] == '.') {
                if (first_dot == null) first_dot = j;
                last_dot = j;
            }
        }

        // If no '.' in basename:
        // - If a path separator was present (e.g. "dir/Makefile"), there is no extension.
        // - Otherwise treat the raw string as an extension (e.g. "WASM" or "json").
        if (last_dot == null) {
            if (has_sep) return null;
            return lookupExactExt(base);
        }

        const ld = last_dot.?;
        const fd = first_dot.?;

        // Try compound extension after first dot (e.g. "app.tar.gz" -> "tar.gz")
        if (fd < ld && fd + 1 < base.len) {
            if (found := lookupExactExt(base[(fd + 1)..])) {
                return found;
            }
        }

        // Fall back to final extension after last dot
        if (ld + 1 >= base.len) return null;
        return lookupExactExt(base[(ld + 1)..]);
    }

    /// Looks up the canonical primary file extension for a MIME type (parameters stripped).
    pub fn toExtension(mime_type: []const u8): ([]const u8)? {
        const essence = extractEssence(mime_type);
        if (essence.len == 0) return null;
        for (entry of getMimeEntries()) {
            if (entry.is_primary && asciiEqlIgnoreCase(entry.mime, essence)) {
                return entry.ext;
            }
        }
        return null;
    }

    /// Returns true if the MIME type represents textual or structured-text data.
    pub fn isText(mime_type: []const u8): bool {
        const essence = extractEssence(mime_type);
        if (essence.len == 0) return false;

        if (asciiStartsWithIgnoreCase(essence, "text/")) return true;
        if (asciiEndsWithIgnoreCase(essence, "+json")) return true;
        if (asciiEndsWithIgnoreCase(essence, "+xml")) return true;
        if (asciiEndsWithIgnoreCase(essence, "+yaml")) return true;
        if (asciiEndsWithIgnoreCase(essence, "+csv")) return true;

        const text_apps: []const []const u8 = [
            "application/json",
            "application/javascript",
            "application/ecmascript",
            "application/typescript",
            "application/xml",
            "application/toml",
            "application/yaml",
            "application/x-yaml",
            "application/sql",
            "application/graphql",
            "application/jsonl",
            "application/x-ndjson",
            "application/x-sh",
            "application/x-www-form-urlencoded",
            "application/x-pem-file",
        ];
        for (t of text_apps) {
            if (asciiEqlIgnoreCase(essence, t)) return true;
        }
        return false;
    }

    /// Returns true if the MIME type benefits from HTTP transport compression (gzip/brotli).
    pub fn isCompressible(mime_type: []const u8): bool {
        const essence = extractEssence(mime_type);
        if (essence.len == 0) return false;
        if (MimeDb.isText(essence)) return true;

        const extra_compressible: []const []const u8 = [
            "application/wasm",
            "application/vnd.ms-fontobject",
            "application/x-tar",
            "application/rtf",
            "font/ttf",
            "font/otf",
            "image/bmp",
            "image/x-icon",
            "image/vnd.microsoft.icon",
            "audio/wav",
            "audio/x-wav",
        ];
        for (c of extra_compressible) {
            if (asciiEqlIgnoreCase(essence, c)) return true;
        }
        return false;
    }
}
