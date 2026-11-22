// Source: Absorbed from /data/dev/bun/src/http_types/MimeType.cp | Synced: 2026-11-22T11:02:38Z
// lib/http/mime.cp
// Complete 180+ entry MIME database, bidirectional lookup, compression/text/charset classification, and magic sniffing

const std = @import("std");

pub enum MimeCategory: u8 {
    None = 0,
    Text = 1,
    Html = 2,
    Css = 3,
    JavaScript = 4,
    Json = 5,
    Xml = 6,
    Image = 7,
    Audio = 8,
    Video = 9,
    Font = 10,
    Application = 11,
    Wasm = 12,
    Model = 13,
    Multipart = 14,
    Message = 15,
    Other = 16,

    pub fn isCode(): bool {
        return switch (_) {
            .Wasm, .Json, .Css, .Html, .JavaScript, .Xml => true,
            else => false,
        };
    }

    pub fn isTextLike(): bool {
        return switch (_) {
            .JavaScript, .Html, .Text, .Css, .Json, .Xml => true,
            else => false,
        };
    }

    pub fn canOpenInEditor(): bool {
        return switch (_) {
            .Text, .Html, .Css, .JavaScript, .Json, .Xml => true,
            else => false,
        };
    }
}

pub struct MimeEntry {
    pub ext: []const u8 = "",
    pub mime: []const u8 = "",
    pub content_type: []const u8 = "",
    pub category: MimeCategory = MimeCategory.Other,
    pub is_text: bool = false,
    pub is_compressible: bool = false,
    pub has_utf8_charset: bool = false,
}

pub const MIME_DATABASE_COUNT: usize = 201;

pub const MIME_DATABASE: [201]MimeEntry = [
    // --- Core Web & Code ---
    MimeEntry{ ext: "html", mime: "text/html", content_type: "text/html; charset=utf-8", category: MimeCategory.Html, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "htm", mime: "text/html", content_type: "text/html; charset=utf-8", category: MimeCategory.Html, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "xhtml", mime: "application/xhtml+xml", content_type: "application/xhtml+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "xht", mime: "application/xhtml+xml", content_type: "application/xhtml+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "css", mime: "text/css", content_type: "text/css; charset=utf-8", category: MimeCategory.Css, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "js", mime: "application/javascript", content_type: "application/javascript; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "mjs", mime: "application/javascript", content_type: "application/javascript; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "cjs", mime: "application/javascript", content_type: "application/javascript; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "jsx", mime: "text/jsx", content_type: "text/jsx; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "ts", mime: "text/typescript", content_type: "text/typescript; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "mts", mime: "text/typescript", content_type: "text/typescript; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "cts", mime: "text/typescript", content_type: "text/typescript; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "tsx", mime: "text/tsx", content_type: "text/tsx; charset=utf-8", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "json", mime: "application/json", content_type: "application/json; charset=utf-8", category: MimeCategory.Json, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "json5", mime: "application/json5", content_type: "application/json5; charset=utf-8", category: MimeCategory.Json, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "jsonld", mime: "application/ld+json", content_type: "application/ld+json", category: MimeCategory.Json, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "geojson", mime: "application/geo+json", content_type: "application/geo+json", category: MimeCategory.Json, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "map", mime: "application/json", content_type: "application/json; charset=utf-8", category: MimeCategory.Json, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "webmanifest", mime: "application/manifest+json", content_type: "application/manifest+json", category: MimeCategory.Json, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "wasm", mime: "application/wasm", content_type: "application/wasm", category: MimeCategory.Wasm, is_text: false, is_compressible: true, has_utf8_charset: false },

    // --- Plain Text, Markup, Config & Docs ---
    MimeEntry{ ext: "txt", mime: "text/plain", content_type: "text/plain; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "text", mime: "text/plain", content_type: "text/plain; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "log", mime: "text/plain", content_type: "text/plain; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "conf", mime: "text/plain", content_type: "text/plain; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "ini", mime: "text/plain", content_type: "text/plain; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "env", mime: "text/plain", content_type: "text/plain; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "md", mime: "text/markdown", content_type: "text/markdown; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "markdown", mime: "text/markdown", content_type: "text/markdown; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "mdx", mime: "text/mdx", content_type: "text/mdx; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "csv", mime: "text/csv", content_type: "text/csv; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "tsv", mime: "text/tab-separated-values", content_type: "text/tab-separated-values; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "yaml", mime: "text/yaml", content_type: "text/yaml; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "yml", mime: "text/yaml", content_type: "text/yaml; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "toml", mime: "application/toml", content_type: "application/toml; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "xml", mime: "application/xml", content_type: "application/xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "xsd", mime: "application/xml", content_type: "application/xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "xsl", mime: "application/xslt+xml", content_type: "application/xslt+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "xslt", mime: "application/xslt+xml", content_type: "application/xslt+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "rss", mime: "application/rss+xml", content_type: "application/rss+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "atom", mime: "application/atom+xml", content_type: "application/atom+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "wsdl", mime: "application/wsdl+xml", content_type: "application/wsdl+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "mathml", mime: "application/mathml+xml", content_type: "application/mathml+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "rdf", mime: "application/rdf+xml", content_type: "application/rdf+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "vtt", mime: "text/vtt", content_type: "text/vtt; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "srt", mime: "application/x-subrip", content_type: "application/x-subrip", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "ics", mime: "text/calendar", content_type: "text/calendar; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "ifb", mime: "text/calendar", content_type: "text/calendar", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "vcf", mime: "text/vcard", content_type: "text/vcard", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "vcard", mime: "text/vcard", content_type: "text/vcard", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "rtf", mime: "application/rtf", content_type: "application/rtf", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },

    // --- Source Code Languages ---
    MimeEntry{ ext: "cp", mime: "text/x-cp", content_type: "text/x-cp; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "zig", mime: "text/x-zig", content_type: "text/x-zig; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "rs", mime: "text/x-rust", content_type: "text/x-rust; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "c", mime: "text/x-c", content_type: "text/x-c; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "h", mime: "text/x-c", content_type: "text/x-c; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "cc", mime: "text/x-c", content_type: "text/x-c; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "cpp", mime: "text/x-c", content_type: "text/x-c; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "hpp", mime: "text/x-c", content_type: "text/x-c; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "py", mime: "text/x-python", content_type: "text/x-python; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "go", mime: "text/x-go", content_type: "text/x-go; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "java", mime: "text/x-java-source", content_type: "text/x-java-source; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "sh", mime: "application/x-sh", content_type: "application/x-sh", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "sql", mime: "application/sql", content_type: "application/sql", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "graphql", mime: "application/graphql", content_type: "application/graphql", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "gql", mime: "application/graphql", content_type: "application/graphql", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "proto", mime: "text/x-protobuf", content_type: "text/x-protobuf", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "tex", mime: "application/x-tex", content_type: "application/x-tex", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "scss", mime: "text/x-scss", content_type: "text/x-scss", category: MimeCategory.Css, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "sass", mime: "text/x-sass", content_type: "text/x-sass", category: MimeCategory.Css, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "less", mime: "text/less", content_type: "text/less", category: MimeCategory.Css, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "stylus", mime: "text/stylus", content_type: "text/stylus", category: MimeCategory.Css, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "coffee", mime: "text/coffeescript", content_type: "text/coffeescript", category: MimeCategory.JavaScript, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "jade", mime: "text/jade", content_type: "text/jade", category: MimeCategory.Html, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "pug", mime: "text/pug", content_type: "text/pug", category: MimeCategory.Html, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "hbs", mime: "text/x-handlebars-template", content_type: "text/x-handlebars-template", category: MimeCategory.Html, is_text: true, is_compressible: true, has_utf8_charset: false },

    // --- Images ---
    MimeEntry{ ext: "png", mime: "image/png", content_type: "image/png", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "jpg", mime: "image/jpeg", content_type: "image/jpeg", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "jpeg", mime: "image/jpeg", content_type: "image/jpeg", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "jpe", mime: "image/jpeg", content_type: "image/jpeg", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "gif", mime: "image/gif", content_type: "image/gif", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "svg", mime: "image/svg+xml", content_type: "image/svg+xml", category: MimeCategory.Image, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "svgz", mime: "image/svg+xml", content_type: "image/svg+xml", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "webp", mime: "image/webp", content_type: "image/webp", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "avif", mime: "image/avif", content_type: "image/avif", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "apng", mime: "image/apng", content_type: "image/apng", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "ico", mime: "image/x-icon", content_type: "image/x-icon", category: MimeCategory.Image, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "cur", mime: "image/x-icon", content_type: "image/x-icon", category: MimeCategory.Image, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "bmp", mime: "image/bmp", content_type: "image/bmp", category: MimeCategory.Image, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "tif", mime: "image/tiff", content_type: "image/tiff", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "tiff", mime: "image/tiff", content_type: "image/tiff", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "heic", mime: "image/heic", content_type: "image/heic", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "heif", mime: "image/heif", content_type: "image/heif", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "jxl", mime: "image/jxl", content_type: "image/jxl", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "psd", mime: "image/vnd.adobe.photoshop", content_type: "image/vnd.adobe.photoshop", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "xbm", mime: "image/x-xbitmap", content_type: "image/x-xbitmap", category: MimeCategory.Image, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "xpm", mime: "image/x-xpixmap", content_type: "image/x-xpixmap", category: MimeCategory.Image, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "dds", mime: "image/vnd.ms-dds", content_type: "image/vnd.ms-dds", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "exr", mime: "image/aces", content_type: "image/aces", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "jp2", mime: "image/jp2", content_type: "image/jp2", category: MimeCategory.Image, is_text: false, is_compressible: false, has_utf8_charset: false },

    // --- Fonts ---
    MimeEntry{ ext: "woff", mime: "font/woff", content_type: "font/woff", category: MimeCategory.Font, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "woff2", mime: "font/woff2", content_type: "font/woff2", category: MimeCategory.Font, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "ttf", mime: "font/ttf", content_type: "font/ttf", category: MimeCategory.Font, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "otf", mime: "font/otf", content_type: "font/otf", category: MimeCategory.Font, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "ttc", mime: "font/collection", content_type: "font/collection", category: MimeCategory.Font, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "eot", mime: "application/vnd.ms-fontobject", content_type: "application/vnd.ms-fontobject", category: MimeCategory.Font, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "sfnt", mime: "font/sfnt", content_type: "font/sfnt", category: MimeCategory.Font, is_text: false, is_compressible: true, has_utf8_charset: false },

    // --- Audio ---
    MimeEntry{ ext: "mp3", mime: "audio/mpeg", content_type: "audio/mpeg", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mp2", mime: "audio/mpeg", content_type: "audio/mpeg", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "m4a", mime: "audio/mp4", content_type: "audio/mp4", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mp4a", mime: "audio/mp4", content_type: "audio/mp4", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "aac", mime: "audio/aac", content_type: "audio/aac", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "ogg", mime: "audio/ogg", content_type: "audio/ogg", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "oga", mime: "audio/ogg", content_type: "audio/ogg", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "opus", mime: "audio/ogg", content_type: "audio/ogg", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "spx", mime: "audio/ogg", content_type: "audio/ogg", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "wav", mime: "audio/wav", content_type: "audio/wav", category: MimeCategory.Audio, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "weba", mime: "audio/webm", content_type: "audio/webm", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "flac", mime: "audio/flac", content_type: "audio/flac", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mid", mime: "audio/midi", content_type: "audio/midi", category: MimeCategory.Audio, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "midi", mime: "audio/midi", content_type: "audio/midi", category: MimeCategory.Audio, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "aif", mime: "audio/x-aiff", content_type: "audio/x-aiff", category: MimeCategory.Audio, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "aiff", mime: "audio/x-aiff", content_type: "audio/x-aiff", category: MimeCategory.Audio, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "au", mime: "audio/basic", content_type: "audio/basic", category: MimeCategory.Audio, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "m3u", mime: "audio/x-mpegurl", content_type: "audio/x-mpegurl", category: MimeCategory.Audio, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "m3u8", mime: "application/vnd.apple.mpegurl", content_type: "application/vnd.apple.mpegurl", category: MimeCategory.Application, is_text: true, is_compressible: true, has_utf8_charset: false },

    // --- Video ---
    MimeEntry{ ext: "mp4", mime: "video/mp4", content_type: "video/mp4", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "m4v", mime: "video/x-m4v", content_type: "video/x-m4v", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "webm", mime: "video/webm", content_type: "video/webm", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "ogv", mime: "video/ogg", content_type: "video/ogg", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mov", mime: "video/quicktime", content_type: "video/quicktime", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "qt", mime: "video/quicktime", content_type: "video/quicktime", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "avi", mime: "video/x-msvideo", content_type: "video/x-msvideo", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mkv", mime: "video/x-matroska", content_type: "video/x-matroska", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "flv", mime: "video/x-flv", content_type: "video/x-flv", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "wmv", mime: "video/x-ms-wmv", content_type: "video/x-ms-wmv", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mpeg", mime: "video/mpeg", content_type: "video/mpeg", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mpg", mime: "video/mpeg", content_type: "video/mpeg", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "3gp", mime: "video/3gpp", content_type: "video/3gpp", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "3g2", mime: "video/3gpp2", content_type: "video/3gpp2", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mpd", mime: "application/dash+xml", content_type: "application/dash+xml", category: MimeCategory.Xml, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "m2ts", mime: "video/mp2t", content_type: "video/mp2t", category: MimeCategory.Video, is_text: false, is_compressible: false, has_utf8_charset: false },

    // --- Archives, Binaries & Documents ---
    MimeEntry{ ext: "pdf", mime: "application/pdf", content_type: "application/pdf", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "ps", mime: "application/postscript", content_type: "application/postscript", category: MimeCategory.Application, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "eps", mime: "application/postscript", content_type: "application/postscript", category: MimeCategory.Application, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "ai", mime: "application/postscript", content_type: "application/postscript", category: MimeCategory.Application, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "zip", mime: "application/zip", content_type: "application/zip", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "tar", mime: "application/x-tar", content_type: "application/x-tar", category: MimeCategory.Application, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "gz", mime: "application/gzip", content_type: "application/gzip", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "tgz", mime: "application/gzip", content_type: "application/gzip", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "bz2", mime: "application/x-bzip2", content_type: "application/x-bzip2", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "xz", mime: "application/x-xz", content_type: "application/x-xz", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "7z", mime: "application/x-7z-compressed", content_type: "application/x-7z-compressed", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "rar", mime: "application/x-rar-compressed", content_type: "application/x-rar-compressed", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "zst", mime: "application/zstd", content_type: "application/zstd", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "br", mime: "application/x-brotli", content_type: "application/x-brotli", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "jar", mime: "application/java-archive", content_type: "application/java-archive", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "war", mime: "application/java-archive", content_type: "application/java-archive", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "apk", mime: "application/vnd.android.package-archive", content_type: "application/vnd.android.package-archive", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "deb", mime: "application/x-debian-package", content_type: "application/x-debian-package", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "rpm", mime: "application/x-redhat-package-manager", content_type: "application/x-redhat-package-manager", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "dmg", mime: "application/x-apple-diskimage", content_type: "application/x-apple-diskimage", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "iso", mime: "application/x-iso9660-image", content_type: "application/x-iso9660-image", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "exe", mime: "application/x-msdownload", content_type: "application/x-msdownload", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "dll", mime: "application/x-msdownload", content_type: "application/x-msdownload", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "msi", mime: "application/x-msdownload", content_type: "application/x-msdownload", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "bin", mime: "application/octet-stream", content_type: "application/octet-stream", category: MimeCategory.Other, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "so", mime: "application/octet-stream", content_type: "application/octet-stream", category: MimeCategory.Other, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "dylib", mime: "application/octet-stream", content_type: "application/octet-stream", category: MimeCategory.Other, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "sqlite", mime: "application/vnd.sqlite3", content_type: "application/vnd.sqlite3", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "db", mime: "application/vnd.sqlite3", content_type: "application/vnd.sqlite3", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },

    // --- Office & E-Books ---
    MimeEntry{ ext: "doc", mime: "application/msword", content_type: "application/msword", category: MimeCategory.Application, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "docx", mime: "application/vnd.openxmlformats-officedocument.wordprocessingml.document", content_type: "application/vnd.openxmlformats-officedocument.wordprocessingml.document", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "xls", mime: "application/vnd.ms-excel", content_type: "application/vnd.ms-excel", category: MimeCategory.Application, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "xlsx", mime: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", content_type: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "ppt", mime: "application/vnd.ms-powerpoint", content_type: "application/vnd.ms-powerpoint", category: MimeCategory.Application, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "pptx", mime: "application/vnd.openxmlformats-officedocument.presentationml.presentation", content_type: "application/vnd.openxmlformats-officedocument.presentationml.presentation", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "odt", mime: "application/vnd.oasis.opendocument.text", content_type: "application/vnd.oasis.opendocument.text", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "ods", mime: "application/vnd.oasis.opendocument.spreadsheet", content_type: "application/vnd.oasis.opendocument.spreadsheet", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "odp", mime: "application/vnd.oasis.opendocument.presentation", content_type: "application/vnd.oasis.opendocument.presentation", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "epub", mime: "application/epub+zip", content_type: "application/epub+zip", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "mobi", mime: "application/x-mobipocket-ebook", content_type: "application/x-mobipocket-ebook", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },

    // --- 3D Models, Email, Certificates & Misc ---
    MimeEntry{ ext: "gltf", mime: "model/gltf+json", content_type: "model/gltf+json", category: MimeCategory.Model, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "glb", mime: "model/gltf-binary", content_type: "model/gltf-binary", category: MimeCategory.Model, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "obj", mime: "model/obj", content_type: "model/obj", category: MimeCategory.Model, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "stl", mime: "model/stl", content_type: "model/stl", category: MimeCategory.Model, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "usdz", mime: "model/vnd.usdz+zip", content_type: "model/vnd.usdz+zip", category: MimeCategory.Model, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "eml", mime: "message/rfc822", content_type: "message/rfc822", category: MimeCategory.Message, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "pem", mime: "application/x-pem-file", content_type: "application/x-pem-file", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "crt", mime: "application/x-x509-ca-cert", content_type: "application/x-x509-ca-cert", category: MimeCategory.Application, is_text: false, is_compressible: true, has_utf8_charset: false },
    MimeEntry{ ext: "der", mime: "application/x-x509-ca-cert", content_type: "application/x-x509-ca-cert", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "p12", mime: "application/x-pkcs12", content_type: "application/x-pkcs12", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "pfx", mime: "application/x-pkcs12", content_type: "application/x-pkcs12", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "swf", mime: "application/x-shockwave-flash", content_type: "application/x-shockwave-flash", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "torrent", mime: "application/x-bittorrent", content_type: "application/x-bittorrent", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "cbor", mime: "application/cbor", content_type: "application/cbor", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "msgpack", mime: "application/msgpack", content_type: "application/msgpack", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "avro", mime: "application/avro", content_type: "application/avro", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "parquet", mime: "application/vnd.apache.parquet", content_type: "application/vnd.apache.parquet", category: MimeCategory.Application, is_text: false, is_compressible: false, has_utf8_charset: false },
    MimeEntry{ ext: "sse", mime: "text/event-stream", content_type: "text/event-stream; charset=utf-8", category: MimeCategory.Text, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "form", mime: "application/x-www-form-urlencoded", content_type: "application/x-www-form-urlencoded; charset=utf-8", category: MimeCategory.Application, is_text: true, is_compressible: true, has_utf8_charset: true },
    MimeEntry{ ext: "multipart", mime: "multipart/form-data", content_type: "multipart/form-data", category: MimeCategory.Multipart, is_text: false, is_compressible: false, has_utf8_charset: false },
];

pub struct MimeType {
    /// Looks up a full `MimeEntry` by file extension (leading `.` is optional, case-insensitive).
    pub static fn lookupExtension(ext: []const u8): MimeEntry? {
        let e = ext;
        if (e.len > 0 && e[0] == '.') {
            e = e[1..];
        }
        if (e.len == 0) return null;
        let i: usize = 0;
        while (i < MIME_DATABASE.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(MIME_DATABASE[i].ext, e)) {
                return MIME_DATABASE[i];
            }
        }
        return null;
    }

    /// Looks up a full `MimeEntry` from a file path by extracting its extension.
    pub static fn lookupPath(path: []const u8): MimeEntry? {
        let dot_idx: usize? = null;
        let i: usize = path.len;
        while (i > 0) {
            i -= 1;
            if (path[i] == '.') {
                dot_idx = i;
                break;
            }
            if (path[i] == '/' || path[i] == '\\') {
                break;
            }
        }
        if (idx := dot_idx) {
            return MimeType.lookupExtension(path[idx + 1 ..]);
        }
        return null;
    }

    /// Strips parameters (e.g. `; charset=utf-8`) and looks up a `MimeEntry` by MIME type string.
    pub static fn lookupMime(mime_str: []const u8): MimeEntry? {
        let base = mime_str;
        if (semi := std.mem.indexOfScalar(u8, mime_str, ';')) {
            base = mime_str[0..semi];
        }
        base = std.mem.trim(u8, base, " \t\r\n");
        if (base.len == 0) return null;

        let i: usize = 0;
        while (i < MIME_DATABASE.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(MIME_DATABASE[i].mime, base)) {
                return MIME_DATABASE[i];
            }
        }
        return null;
    }

    /// Reverse lookup: returns the canonical file extension for a MIME type (e.g. `"image/png"` -> `"png"`).
    pub static fn extensionForMime(mime_str: []const u8): ([]const u8)? {
        if (entry := MimeType.lookupMime(mime_str)) {
            return entry.ext;
        }
        return null;
    }

    /// Returns the Content-Type string for a file extension, falling back to `"application/octet-stream"`.
    pub static fn fromExtension(ext: []const u8): []const u8 {
        if (entry := MimeType.lookupExtension(ext)) {
            return entry.content_type;
        }
        return "application/octet-stream";
    }

    /// Returns the Content-Type string for a file path, falling back to `"application/octet-stream"`.
    pub static fn fromPath(path: []const u8): []const u8 {
        if (entry := MimeType.lookupPath(path)) {
            return entry.content_type;
        }
        return "application/octet-stream";
    }

    fn resolveQuery(mime_or_ext: []const u8): MimeEntry? {
        if (std.mem.indexOfScalar(u8, mime_or_ext, '/') != null) {
            return MimeType.lookupMime(mime_or_ext);
        }
        return MimeType.lookupExtension(mime_or_ext);
    }

    /// Returns true if the given MIME type or extension benefits from HTTP compression (gzip/deflate/br).
    pub static fn isCompressible(mime_or_ext: []const u8): bool {
        if (entry := MimeType.resolveQuery(mime_or_ext)) {
            return entry.is_compressible;
        }
        let base = mime_or_ext;
        if (semi := std.mem.indexOfScalar(u8, base, ';')) {
            base = base[0..semi];
        }
        base = std.mem.trim(u8, base, " \t");
        if (std.ascii.startsWithIgnoreCase(base, "text/")) return true;
        if (std.ascii.endsWithIgnoreCase(base, "+xml") || std.ascii.endsWithIgnoreCase(base, "+json")) return true;
        return false;
    }

    /// Returns true if the given MIME type or extension is human-readable text.
    pub static fn isText(mime_or_ext: []const u8): bool {
        if (entry := MimeType.resolveQuery(mime_or_ext)) {
            return entry.is_text;
        }
        let base = mime_or_ext;
        if (semi := std.mem.indexOfScalar(u8, base, ';')) {
            base = base[0..semi];
        }
        base = std.mem.trim(u8, base, " \t");
        return std.ascii.startsWithIgnoreCase(base, "text/") ||
            std.ascii.endsWithIgnoreCase(base, "+xml") ||
            std.ascii.endsWithIgnoreCase(base, "+json");
    }

    static fn asciiContainsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
        if (needle.len == 0) return true;
        if (haystack.len < needle.len) return false;
        let i: usize = 0;
        while (i + needle.len <= haystack.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) return true;
        }
        return false;
    }

    /// Returns true if the MIME type defaults to UTF-8 charset or explicitly declares UTF-8.
    pub static fn isUtf8Charset(mime_or_ext: []const u8): bool {
        if (std.mem.indexOfScalar(u8, mime_or_ext, ';') != null) {
            if (MimeType.asciiContainsIgnoreCase(mime_or_ext, "charset=utf-8") ||
                MimeType.asciiContainsIgnoreCase(mime_or_ext, "charset=utf8") ||
                MimeType.asciiContainsIgnoreCase(mime_or_ext, "charset=\"utf-8\""))
            {
                return true;
            }
        }
        if (entry := MimeType.resolveQuery(mime_or_ext)) {
            return entry.has_utf8_charset;
        }
        return false;
    }

    /// Returns the high-level `MimeCategory` for a MIME type or file extension.
    pub static fn categoryOf(mime_or_ext: []const u8): MimeCategory {
        if (entry := MimeType.resolveQuery(mime_or_ext)) {
            return entry.category;
        }
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "text/")) return MimeCategory.Text;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "image/")) return MimeCategory.Image;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "audio/")) return MimeCategory.Audio;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "video/")) return MimeCategory.Video;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "font/")) return MimeCategory.Font;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "application/")) return MimeCategory.Application;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "model/")) return MimeCategory.Model;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "multipart/")) return MimeCategory.Multipart;
        if (std.ascii.startsWithIgnoreCase(mime_or_ext, "message/")) return MimeCategory.Message;
        return MimeCategory.Other;
    }

    /// Sniffs common binary/text magic signatures (PNG, JPEG, GIF, WebP, BMP, TIFF, ICO, WASM, PDF, GZIP, ZIP, OGG, HTML, SVG).
    pub static fn sniffMagic(bytes: []const u8): MimeEntry? {
        if (bytes.len >= 8 && std.mem.eql(u8, bytes[0..8], "\x89PNG\r\n\x1A\n")) {
            return MimeType.lookupExtension("png");
        }
        if (bytes.len >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
            return MimeType.lookupExtension("jpg");
        }
        if (bytes.len >= 6 && (std.mem.eql(u8, bytes[0..6], "GIF89a") || std.mem.eql(u8, bytes[0..6], "GIF87a"))) {
            return MimeType.lookupExtension("gif");
        }
        if (bytes.len >= 12 && std.mem.eql(u8, bytes[0..4], "RIFF") && std.mem.eql(u8, bytes[8..12], "WEBP")) {
            return MimeType.lookupExtension("webp");
        }
        if (bytes.len >= 2 && bytes[0] == 'B' && bytes[1] == 'M') {
            return MimeType.lookupExtension("bmp");
        }
        if (bytes.len >= 4 && (std.mem.eql(u8, bytes[0..4], "II*\x00") || std.mem.eql(u8, bytes[0..4], "MM\x00*"))) {
            return MimeType.lookupExtension("tiff");
        }
        if (bytes.len >= 4 && bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x01 && bytes[3] == 0x00) {
            return MimeType.lookupExtension("ico");
        }
        if (bytes.len >= 4 && std.mem.eql(u8, bytes[0..4], "\x00asm")) {
            return MimeType.lookupExtension("wasm");
        }
        if (bytes.len >= 5 && std.mem.eql(u8, bytes[0..5], "%PDF-")) {
            return MimeType.lookupExtension("pdf");
        }
        if (bytes.len >= 2 && bytes[0] == 0x1F && bytes[1] == 0x8B) {
            return MimeType.lookupExtension("gz");
        }
        if (bytes.len >= 4 && bytes[0] == 'P' && bytes[1] == 'K' && bytes[2] == 0x03 && bytes[3] == 0x04) {
            return MimeType.lookupExtension("zip");
        }
        if (bytes.len >= 4 && std.mem.eql(u8, bytes[0..4], "OggS")) {
            return MimeType.lookupExtension("ogg");
        }
        if (bytes.len >= 3 && std.mem.eql(u8, bytes[0..3], "ID3")) {
            return MimeType.lookupExtension("mp3");
        }
        const trimmed = std.mem.trimStart(u8, bytes, " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(trimmed, "<!DOCTYPE html") || std.ascii.startsWithIgnoreCase(trimmed, "<html")) {
            return MimeType.lookupExtension("html");
        }
        if (std.ascii.startsWithIgnoreCase(trimmed, "<svg")) {
            return MimeType.lookupExtension("svg");
        }
        return null;
    }
}
