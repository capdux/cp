// @cap/mime — Pure CP, zero-dependency, zero-heap-allocation IANA MIME type registry
// Clean-room MIT implementation based on the public IANA Media Types registry,
// inspired by npm `mime-types` and Rust `mime_guess`.
// Features:
//   - Zero heap allocation, zero `unsafe`, 100% static compile-time lookup tables
//   - 100+ production IANA extensions (Web, fonts, audio/video, documents, archives, binaries, source code)
//   - Extension & file-path lookup (case-insensitive, supports "json", ".json", "/a/b/file.min.js")
//   - Reverse MIME-to-extension lookup with automatic parameter stripping ("; charset=...")
//   - Default charset & compressibility detection following IANA / mime-db conventions
//   - `Content-Type` header formatting with automatic `; charset=utf-8` completion and buffer bounds guard

pub err MimeError {
    BufferTooSmall,
    InvalidMime,
}

pub struct MimeInfo {
    mime: []const u8;
    default_ext: []const u8;
    compressible: bool;
    utf8_charset: bool;
}

struct ExtEntry {
    ext: []const u8;
    info: MimeInfo;
}

struct MimeAlias {
    alias: []const u8;
    info: MimeInfo;
}

const OCTET_STREAM: []const u8 = "application/octet-stream";
const UTF8_CHARSET: []const u8 = "utf-8";
const CHARSET_SUFFIX: []const u8 = "; charset=utf-8";

// Core IANA Media Types table (109 extensions)
const EXT_TABLE: [109]ExtEntry = [
    // --- Web, Markup, Styles, Scripts, Data & Images ---
    ExtEntry { ext: "html",        info: MimeInfo { mime: "text/html",                                                              default_ext: "html",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "htm",         info: MimeInfo { mime: "text/html",                                                              default_ext: "html",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "css",         info: MimeInfo { mime: "text/css",                                                               default_ext: "css",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "js",          info: MimeInfo { mime: "text/javascript",                                                        default_ext: "js",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "mjs",         info: MimeInfo { mime: "text/javascript",                                                        default_ext: "js",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "cjs",         info: MimeInfo { mime: "text/javascript",                                                        default_ext: "js",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "tsx",         info: MimeInfo { mime: "text/typescript",                                                        default_ext: "tsx",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "mts",         info: MimeInfo { mime: "text/typescript",                                                        default_ext: "tsx",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "cts",         info: MimeInfo { mime: "text/typescript",                                                        default_ext: "tsx",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "json",        info: MimeInfo { mime: "application/json",                                                       default_ext: "json",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "map",         info: MimeInfo { mime: "application/json",                                                       default_ext: "json",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "jsonld",      info: MimeInfo { mime: "application/ld+json",                                                    default_ext: "jsonld",      compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "ndjson",      info: MimeInfo { mime: "application/x-ndjson",                                                   default_ext: "ndjson",      compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "jsonl",       info: MimeInfo { mime: "application/x-ndjson",                                                   default_ext: "ndjson",      compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "webmanifest", info: MimeInfo { mime: "application/manifest+json",                                              default_ext: "webmanifest", compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "xml",         info: MimeInfo { mime: "application/xml",                                                        default_ext: "xml",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "xhtml",       info: MimeInfo { mime: "application/xhtml+xml",                                                  default_ext: "xhtml",       compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "rss",         info: MimeInfo { mime: "application/rss+xml",                                                    default_ext: "rss",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "atom",        info: MimeInfo { mime: "application/atom+xml",                                                   default_ext: "atom",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "svg",         info: MimeInfo { mime: "image/svg+xml",                                                          default_ext: "svg",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "wasm",        info: MimeInfo { mime: "application/wasm",                                                       default_ext: "wasm",        compressible: true,  utf8_charset: false } },
    ExtEntry { ext: "txt",         info: MimeInfo { mime: "text/plain",                                                             default_ext: "txt",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "text",        info: MimeInfo { mime: "text/plain",                                                             default_ext: "txt",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "log",         info: MimeInfo { mime: "text/plain",                                                             default_ext: "txt",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "md",          info: MimeInfo { mime: "text/markdown",                                                          default_ext: "md",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "markdown",    info: MimeInfo { mime: "text/markdown",                                                          default_ext: "md",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "csv",         info: MimeInfo { mime: "text/csv",                                                               default_ext: "csv",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "tsv",         info: MimeInfo { mime: "text/tab-separated-values",                                              default_ext: "tsv",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "yaml",        info: MimeInfo { mime: "application/yaml",                                                       default_ext: "yaml",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "yml",         info: MimeInfo { mime: "application/yaml",                                                       default_ext: "yaml",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "toml",        info: MimeInfo { mime: "application/toml",                                                       default_ext: "toml",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "ico",         info: MimeInfo { mime: "image/vnd.microsoft.icon",                                               default_ext: "ico",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "webp",        info: MimeInfo { mime: "image/webp",                                                             default_ext: "webp",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "avif",        info: MimeInfo { mime: "image/avif",                                                             default_ext: "avif",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "png",         info: MimeInfo { mime: "image/png",                                                              default_ext: "png",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "jpg",         info: MimeInfo { mime: "image/jpeg",                                                             default_ext: "jpg",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "jpeg",        info: MimeInfo { mime: "image/jpeg",                                                             default_ext: "jpg",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "gif",         info: MimeInfo { mime: "image/gif",                                                              default_ext: "gif",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "bmp",         info: MimeInfo { mime: "image/bmp",                                                              default_ext: "bmp",         compressible: true,  utf8_charset: false } },
    ExtEntry { ext: "tiff",        info: MimeInfo { mime: "image/tiff",                                                             default_ext: "tiff",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "tif",         info: MimeInfo { mime: "image/tiff",                                                             default_ext: "tiff",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "heic",        info: MimeInfo { mime: "image/heic",                                                             default_ext: "heic",        compressible: false, utf8_charset: false } },

    // --- Fonts ---
    ExtEntry { ext: "woff",        info: MimeInfo { mime: "font/woff",                                                              default_ext: "woff",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "woff2",       info: MimeInfo { mime: "font/woff2",                                                             default_ext: "woff2",       compressible: false, utf8_charset: false } },
    ExtEntry { ext: "ttf",         info: MimeInfo { mime: "font/ttf",                                                               default_ext: "ttf",         compressible: true,  utf8_charset: false } },
    ExtEntry { ext: "otf",         info: MimeInfo { mime: "font/otf",                                                               default_ext: "otf",         compressible: true,  utf8_charset: false } },
    ExtEntry { ext: "eot",         info: MimeInfo { mime: "application/vnd.ms-fontobject",                                          default_ext: "eot",         compressible: true,  utf8_charset: false } },

    // --- Audio & Video ---
    ExtEntry { ext: "mp3",         info: MimeInfo { mime: "audio/mpeg",                                                             default_ext: "mp3",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "wav",         info: MimeInfo { mime: "audio/wav",                                                              default_ext: "wav",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "ogg",         info: MimeInfo { mime: "audio/ogg",                                                              default_ext: "ogg",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "oga",         info: MimeInfo { mime: "audio/ogg",                                                              default_ext: "ogg",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "ogv",         info: MimeInfo { mime: "video/ogg",                                                              default_ext: "ogv",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "flac",        info: MimeInfo { mime: "audio/flac",                                                             default_ext: "flac",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "aac",         info: MimeInfo { mime: "audio/aac",                                                              default_ext: "aac",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "m4a",         info: MimeInfo { mime: "audio/mp4",                                                              default_ext: "m4a",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "mp4",         info: MimeInfo { mime: "video/mp4",                                                              default_ext: "mp4",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "webm",        info: MimeInfo { mime: "video/webm",                                                             default_ext: "webm",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "mkv",         info: MimeInfo { mime: "video/x-matroska",                                                       default_ext: "mkv",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "avi",         info: MimeInfo { mime: "video/x-msvideo",                                                        default_ext: "avi",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "mov",         info: MimeInfo { mime: "video/quicktime",                                                        default_ext: "mov",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "m3u8",        info: MimeInfo { mime: "application/vnd.apple.mpegurl",                                          default_ext: "m3u8",        compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "ts",          info: MimeInfo { mime: "video/mp2t",                                                             default_ext: "ts",          compressible: false, utf8_charset: false } },
    ExtEntry { ext: "m2ts",        info: MimeInfo { mime: "video/mp2t",                                                             default_ext: "ts",          compressible: false, utf8_charset: false } },

    // --- Documents, Archives, Binaries & Source Code ---
    ExtEntry { ext: "pdf",         info: MimeInfo { mime: "application/pdf",                                                        default_ext: "pdf",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "zip",         info: MimeInfo { mime: "application/zip",                                                        default_ext: "zip",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "gz",          info: MimeInfo { mime: "application/gzip",                                                       default_ext: "gz",          compressible: false, utf8_charset: false } },
    ExtEntry { ext: "gzip",        info: MimeInfo { mime: "application/gzip",                                                       default_ext: "gz",          compressible: false, utf8_charset: false } },
    ExtEntry { ext: "tar",         info: MimeInfo { mime: "application/x-tar",                                                      default_ext: "tar",         compressible: true,  utf8_charset: false } },
    ExtEntry { ext: "tgz",         info: MimeInfo { mime: "application/gzip",                                                       default_ext: "gz",          compressible: false, utf8_charset: false } },
    ExtEntry { ext: "bz2",         info: MimeInfo { mime: "application/x-bzip2",                                                    default_ext: "bz2",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "xz",          info: MimeInfo { mime: "application/x-xz",                                                       default_ext: "xz",          compressible: false, utf8_charset: false } },
    ExtEntry { ext: "zst",         info: MimeInfo { mime: "application/zstd",                                                       default_ext: "zst",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "7z",          info: MimeInfo { mime: "application/x-7z-compressed",                                            default_ext: "7z",          compressible: false, utf8_charset: false } },
    ExtEntry { ext: "rar",         info: MimeInfo { mime: "application/vnd.rar",                                                    default_ext: "rar",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "bin",         info: MimeInfo { mime: "application/octet-stream",                                               default_ext: "bin",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "dll",         info: MimeInfo { mime: "application/octet-stream",                                               default_ext: "bin",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "so",          info: MimeInfo { mime: "application/octet-stream",                                               default_ext: "bin",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "dylib",       info: MimeInfo { mime: "application/octet-stream",                                               default_ext: "bin",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "exe",         info: MimeInfo { mime: "application/vnd.microsoft.portable-executable",                          default_ext: "exe",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "dmg",         info: MimeInfo { mime: "application/x-apple-diskimage",                                          default_ext: "dmg",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "deb",         info: MimeInfo { mime: "application/vnd.debian.binary-package",                                  default_ext: "deb",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "rpm",         info: MimeInfo { mime: "application/x-redhat-package-manager",                                   default_ext: "rpm",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "apk",         info: MimeInfo { mime: "application/vnd.android.package-archive",                                default_ext: "apk",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "doc",         info: MimeInfo { mime: "application/msword",                                                     default_ext: "doc",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "docx",        info: MimeInfo { mime: "application/vnd.openxmlformats-officedocument.wordprocessingml.document", default_ext: "docx",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "xls",         info: MimeInfo { mime: "application/vnd.ms-excel",                                               default_ext: "xls",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "xlsx",        info: MimeInfo { mime: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",      default_ext: "xlsx",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "ppt",         info: MimeInfo { mime: "application/vnd.ms-powerpoint",                                          default_ext: "ppt",         compressible: false, utf8_charset: false } },
    ExtEntry { ext: "pptx",        info: MimeInfo { mime: "application/vnd.openxmlformats-officedocument.presentationml.presentation", default_ext: "pptx",     compressible: false, utf8_charset: false } },
    ExtEntry { ext: "epub",        info: MimeInfo { mime: "application/epub+zip",                                                   default_ext: "epub",        compressible: false, utf8_charset: false } },
    ExtEntry { ext: "rtf",         info: MimeInfo { mime: "application/rtf",                                                        default_ext: "rtf",         compressible: true,  utf8_charset: false } },
    ExtEntry { ext: "ics",         info: MimeInfo { mime: "text/calendar",                                                          default_ext: "ics",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "vcf",         info: MimeInfo { mime: "text/vcard",                                                             default_ext: "vcf",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "sql",         info: MimeInfo { mime: "application/sql",                                                        default_ext: "sql",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "graphql",     info: MimeInfo { mime: "application/graphql",                                                    default_ext: "graphql",     compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "gql",         info: MimeInfo { mime: "application/graphql",                                                    default_ext: "graphql",     compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "proto",       info: MimeInfo { mime: "text/x-protobuf",                                                        default_ext: "proto",       compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "cp",          info: MimeInfo { mime: "text/x-cp",                                                              default_ext: "cp",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "zig",         info: MimeInfo { mime: "text/x-zig",                                                             default_ext: "zig",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "rs",          info: MimeInfo { mime: "text/x-rust",                                                            default_ext: "rs",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "py",          info: MimeInfo { mime: "text/x-python",                                                          default_ext: "py",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "go",          info: MimeInfo { mime: "text/x-go",                                                              default_ext: "go",          compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "c",           info: MimeInfo { mime: "text/x-c",                                                               default_ext: "c",           compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "h",           info: MimeInfo { mime: "text/x-c-header",                                                        default_ext: "h",           compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "cpp",         info: MimeInfo { mime: "text/x-c++",                                                             default_ext: "cpp",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "cc",          info: MimeInfo { mime: "text/x-c++",                                                             default_ext: "cpp",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "cxx",         info: MimeInfo { mime: "text/x-c++",                                                             default_ext: "cpp",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "hpp",         info: MimeInfo { mime: "text/x-c++",                                                             default_ext: "cpp",         compressible: true,  utf8_charset: true  } },
    ExtEntry { ext: "sh",          info: MimeInfo { mime: "application/x-sh",                                                       default_ext: "sh",          compressible: true,  utf8_charset: true  } },
];

// Common historical / legacy MIME aliases for reverse lookup, charset, and compressibility
const MIME_ALIASES: [12]MimeAlias = [
    MimeAlias { alias: "application/javascript",   info: MimeInfo { mime: "text/javascript",          default_ext: "js",   compressible: true,  utf8_charset: true  } },
    MimeAlias { alias: "application/x-javascript", info: MimeInfo { mime: "text/javascript",          default_ext: "js",   compressible: true,  utf8_charset: true  } },
    MimeAlias { alias: "application/typescript",   info: MimeInfo { mime: "text/typescript",          default_ext: "tsx",  compressible: true,  utf8_charset: true  } },
    MimeAlias { alias: "text/xml",                 info: MimeInfo { mime: "application/xml",          default_ext: "xml",  compressible: true,  utf8_charset: true  } },
    MimeAlias { alias: "text/yaml",                info: MimeInfo { mime: "application/yaml",         default_ext: "yaml", compressible: true,  utf8_charset: true  } },
    MimeAlias { alias: "application/x-yaml",       info: MimeInfo { mime: "application/yaml",         default_ext: "yaml", compressible: true,  utf8_charset: true  } },
    MimeAlias { alias: "text/x-yaml",              info: MimeInfo { mime: "application/yaml",         default_ext: "yaml", compressible: true,  utf8_charset: true  } },
    MimeAlias { alias: "image/x-icon",             info: MimeInfo { mime: "image/vnd.microsoft.icon", default_ext: "ico",  compressible: false, utf8_charset: false } },
    MimeAlias { alias: "image/jpg",                info: MimeInfo { mime: "image/jpeg",               default_ext: "jpg",  compressible: false, utf8_charset: false } },
    MimeAlias { alias: "application/x-gzip",       info: MimeInfo { mime: "application/gzip",         default_ext: "gz",   compressible: false, utf8_charset: false } },
    MimeAlias { alias: "application/x-zip-compressed", info: MimeInfo { mime: "application/zip",      default_ext: "zip",  compressible: false, utf8_charset: false } },
    MimeAlias { alias: "application/x-www-form-urlencoded", info: MimeInfo { mime: "application/x-www-form-urlencoded", default_ext: "", compressible: true, utf8_charset: true } },
];

fn isAsciiSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn toAsciiLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + 32;
    }
    return b;
}

fn isTokenChar(b: u8): bool {
    if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9')) {
        return true;
    }
    return b == '!' || b == '#' || b == '$' || b == '%' || b == '&' || b == '\'' ||
           b == '*' || b == '+' || b == '-' || b == '.' || b == '^' || b == '_' ||
           b == '`' || b == '|' || b == '~';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiSpace(s[start])) : (start += 1) {}
    while (end > start && isAsciiSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toAsciiLower(a[i]) != toAsciiLower(b[i])) {
            return false;
        }
    }
    return true;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return eqlIgnoreCase(s[0..prefix.len], prefix);
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) {
        return false;
    }
    return eqlIgnoreCase(s[s.len - suffix.len .. s.len], suffix);
}

fn findByte(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == target) {
            return i;
        }
    }
    return null;
}

// Extract normalized extension slice from bare extension ("json"), dotted extension (".json"),
// or full path ("/a/b/file.min.js", "C:\\dir\\doc.PDF").
fn extractExtension(ext_or_path: []const u8): ([]const u8)? {
    const trimmed = trimAscii(ext_or_path);
    if (trimmed.len == 0) {
        return null;
    }

    // Locate last path separator ('/' or '\\')
    let has_sep = false;
    let base_start: usize = 0;
    let i: usize = 0;
    while (i < trimmed.len) : (i += 1) {
        if (trimmed[i] == '/' || trimmed[i] == '\\') {
            has_sep = true;
            base_start = i + 1;
        }
    }

    if (base_start >= trimmed.len) {
        return null;
    }
    const base = trimmed[base_start..trimmed.len];

    // Locate last '.' in basename
    let has_dot = false;
    let last_dot: usize = 0;
    let j: usize = 0;
    while (j < base.len) : (j += 1) {
        if (base[j] == '.') {
            has_dot = true;
            last_dot = j;
        }
    }

    if (!has_dot) {
        // A path without an extension (e.g. "/a/b/Makefile") has no extension
        if (has_sep) {
            return null;
        }
        // Bare extension (e.g. "json", "HTML")
        return base;
    }

    // Trailing dot (e.g. "file." or ".")
    if (last_dot + 1 >= base.len) {
        return null;
    }

    // Leading dot in a path is a hidden dotfile (e.g. "/a/.gitignore"), not a dotted extension
    if (last_dot == 0 && has_sep) {
        return null;
    }

    return base[last_dot + 1 .. base.len];
}

// Strip `; ...` parameters and trim surrounding ASCII whitespace.
fn stripMimeParams(mime: []const u8): []const u8 {
    if (semi := findByte(mime, ';')) {
        return trimAscii(mime[0..semi]);
    }
    return trimAscii(mime);
}

// Validate RFC 9110 `type/subtype` syntax.
fn isValidMimeEssence(essence: []const u8): bool {
    if (essence.len < 3) {
        return false;
    }
    let slash_count: usize = 0;
    let slash_pos: usize = 0;
    let i: usize = 0;
    while (i < essence.len) : (i += 1) {
        const ch = essence[i];
        if (ch == '/') {
            slash_count += 1;
            slash_pos = i;
        } else if (!isTokenChar(ch)) {
            return false;
        }
    }
    if (slash_count != 1) {
        return false;
    }
    return slash_pos > 0 && slash_pos + 1 < essence.len;
}

fn isKnownTopLevelType(t: []const u8): bool {
    return eqlIgnoreCase(t, "text") ||
           eqlIgnoreCase(t, "application") ||
           eqlIgnoreCase(t, "image") ||
           eqlIgnoreCase(t, "audio") ||
           eqlIgnoreCase(t, "video") ||
           eqlIgnoreCase(t, "font") ||
           eqlIgnoreCase(t, "multipart") ||
           eqlIgnoreCase(t, "message") ||
           eqlIgnoreCase(t, "model") ||
           eqlIgnoreCase(t, "example") ||
           startsWithIgnoreCase(t, "x-");
}

// Check if parameter section already contains `charset=` (case-insensitive).
fn hasCharsetParam(params: []const u8): bool {
    let pos: usize = 0;
    while (pos < params.len) {
        // Advance past ';' and whitespace
        while (pos < params.len && (params[pos] == ';' || isAsciiSpace(params[pos]))) : (pos += 1) {}
        if (pos >= params.len) {
            return false;
        }
        const param_start = pos;
        while (pos < params.len && params[pos] != ';') : (pos += 1) {}
        const seg = trimAscii(params[param_start..pos]);
        if (startsWithIgnoreCase(seg, "charset")) {
            let k: usize = 7;
            while (k < seg.len && isAsciiSpace(seg[k])) : (k += 1) {}
            if (k < seg.len && seg[k] == '=') {
                return true;
            }
        }
    }
    return false;
}

fn copySlice(dst: []u8, offset: usize, src: []const u8): usize ! MimeError {
    if (offset + src.len > dst.len) {
        return MimeError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

/// Lookup full `MimeInfo` metadata by extension (e.g. `"json"`, `".json"`) or file path (`"/a/b/file.min.js"`).
pub fn lookupInfo(ext_or_path: []const u8): MimeInfo? {
    const ext = extractExtension(ext_or_path) ?? return null;
    let i: usize = 0;
    while (i < EXT_TABLE.len) : (i += 1) {
        const entry = EXT_TABLE[i];
        if (entry.ext.len == ext.len && eqlIgnoreCase(entry.ext, ext)) {
            return entry.info;
        }
    }
    return null;
}

/// Lookup full `MimeInfo` metadata by MIME type (strips `; charset=...`, case-insensitive).
pub fn lookupInfoByMime(mime: []const u8): MimeInfo? {
    const essence = stripMimeParams(mime);
    if (essence.len == 0) {
        return null;
    }

    let i: usize = 0;
    while (i < EXT_TABLE.len) : (i += 1) {
        const info = EXT_TABLE[i].info;
        if (info.mime.len == essence.len && eqlIgnoreCase(info.mime, essence)) {
            return info;
        }
    }

    let j: usize = 0;
    while (j < MIME_ALIASES.len) : (j += 1) {
        const item = MIME_ALIASES[j];
        if (item.alias.len == essence.len && eqlIgnoreCase(item.alias, essence)) {
            return item.info;
        }
    }

    return null;
}

/// Lookup MIME type by extension (`"json"`, `".json"`) or file path (`"/a/b/file.min.js"`).
/// Case-insensitive; returns `null` when unknown.
pub fn lookupByExt(ext_or_path: []const u8): ([]const u8)? {
    if (info := lookupInfo(ext_or_path)) {
        return info.mime;
    }
    return null;
}

/// Lookup MIME type by extension or file path, falling back to `"application/octet-stream"`.
pub fn lookupOrOctetStream(ext_or_path: []const u8): []const u8 {
    return lookupByExt(ext_or_path) ?? OCTET_STREAM;
}

/// Reverse lookup canonical file extension (without leading dot) for a MIME type.
/// Automatically strips `; charset=...` parameters; case-insensitive.
pub fn extensionForMime(mime: []const u8): ([]const u8)? {
    if (info := lookupInfoByMime(mime)) {
        if (info.default_ext.len == 0) {
            return null;
        }
        return info.default_ext;
    }
    return null;
}

/// Return default charset (`"utf-8"`) for textual / JSON / JS / XML / SVG / YAML MIME types,
/// or `null` for binary types.
pub fn defaultCharset(mime: []const u8): ([]const u8)? {
    const essence = stripMimeParams(mime);
    if (essence.len == 0) {
        return null;
    }

    if (info := lookupInfoByMime(essence)) {
        if (info.utf8_charset) {
            return UTF8_CHARSET;
        }
        return null;
    }

    // IANA / RFC structured syntax heuristics for unregistered text/+json/+xml types
    if (startsWithIgnoreCase(essence, "text/") && essence.len > 5) {
        return UTF8_CHARSET;
    }
    if (endsWithIgnoreCase(essence, "+json") || endsWithIgnoreCase(essence, "+xml")) {
        return UTF8_CHARSET;
    }

    return null;
}

/// Determine whether a MIME type is compressible (gzip / brotli / zstd) per IANA / mime-db conventions.
pub fn isCompressible(mime: []const u8): bool {
    const essence = stripMimeParams(mime);
    if (!isValidMimeEssence(essence)) {
        return false;
    }

    if (info := lookupInfoByMime(essence)) {
        return info.compressible;
    }

    // General IANA / mime-db heuristics for custom structured media types
    if (startsWithIgnoreCase(essence, "text/")) {
        return true;
    }
    if (endsWithIgnoreCase(essence, "+json") ||
        endsWithIgnoreCase(essence, "+xml") ||
        endsWithIgnoreCase(essence, "+text")) {
        return true;
    }

    return false;
}

/// Build an HTTP `Content-Type` header value into caller-supplied buffer `out`.
/// Accepts either an extension/path (e.g. `"json"`, `".html"`, `"/static/app.js"`) or a MIME type
/// (e.g. `"text/html"`, `"application/json"`, `"text/plain; charset=iso-8859-1"`).
/// Automatically appends `; charset=utf-8` when appropriate and not already present.
pub fn contentTypeHeader(ext_or_mime: []const u8, out: []u8): ([]const u8) ! MimeError {
    const trimmed = trimAscii(ext_or_mime);
    if (trimmed.len == 0) {
        return MimeError.InvalidMime;
    }

    // Split optional `; params`
    let essence = trimmed;
    let params: []const u8 = "";
    let has_params = false;
    if (semi := findByte(trimmed, ';')) {
        essence = trimAscii(trimmed[0..semi]);
        params = trimmed[semi..trimmed.len];
        has_params = true;
    }

    // Determine whether input is a direct MIME type or a file path / extension
    let treat_as_mime = false;
    if (isValidMimeEssence(essence)) {
        if (slash := findByte(essence, '/')) {
            const top_type = essence[0..slash];
            if (isKnownTopLevelType(top_type)) {
                treat_as_mime = true;
            }
        }
    }

    if (treat_as_mime) {
        let pos: usize = 0;
        pos = copySlice(out, pos, trimmed)?;
        if (!has_params || !hasCharsetParam(params)) {
            if (defaultCharset(essence) != null) {
                pos = copySlice(out, pos, CHARSET_SUFFIX)?;
            }
        }
        return out[0..pos];
    }

    // Otherwise, reject inputs with `;` that aren't valid MIME types
    if (has_params) {
        return MimeError.InvalidMime;
    }

    // Resolve as extension or path
    const resolved_mime = lookupByExt(trimmed) ?? return MimeError.InvalidMime;
    let pos: usize = 0;
    pos = copySlice(out, pos, resolved_mime)?;
    if (defaultCharset(resolved_mime) != null) {
        pos = copySlice(out, pos, CHARSET_SUFFIX)?;
    }
    return out[0..pos];
}
