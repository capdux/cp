// @cap/mime-types — Zero-heap MIME type / extension / charset / Content-Type resolver in pure CP
// Clean-room MIT implementation inspired by npm `mime-types` v2.1.

import { isAlnum, eqIgnoreCase, trim, findByte, rfindByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub struct MimeParts {
    pub valid: bool,
    pub essence: string,
    pub type_part: string,
    pub subtype: string,
    pub charset_val: string,
}

// ============================================================================
// Private helpers (all prefixed with `mtyp` to avoid module flattening clashes)
// ============================================================================

fn mtypIsTokenChar(c: u8): bool {
    if (isAlnum(c)) {
        return true;
    }
    return c == 33 || c == 35 || c == 36 || c == 38 || c == 43 || c == 45 ||
        c == 46 || c == 94 || c == 95 || c == 96 || c == 124 || c == 126;
}

fn mtypEndsWithIgnoreCase(s: string, suffix: string): bool {
    if (s.len < suffix.len) {
        return false;
    }
    return eqIgnoreCase(s[(s.len - suffix.len)..s.len], suffix);
}

fn mtypIsValidToken(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!mtypIsTokenChar(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn mtypExtractExt(path_or_ext: string): string? {
    const s: string = trim(path_or_ext);
    if (s.len == 0) {
        return null;
    }

    // Find last path separator ('/' or '\\')
    let base_start: usize = 0;
    let has_sep: bool = false;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 47 || s[i] == 92) {
            base_start = i + 1;
            has_sep = true;
        }
        i = i + 1;
    }
    if (base_start >= s.len) {
        return null;
    }
    const base: string = s[base_start..s.len];

    // Find last '.' in basename
    if (dot_idx := rfindByte(base, 46)) {
        if (dot_idx + 1 >= base.len) {
            return null;
        }
        return base[(dot_idx + 1)..base.len];
    }
    if (has_sep) {
        return null;
    }
    return base;
}

// ============================================================================
// MIME Type Database (95+ Web / Dev / Media / Font / Document / Archive types)
// ============================================================================

fn mtypLookupExt(ext: string): string? {
    // Web & Markup
    if (eqIgnoreCase(ext, "html") || eqIgnoreCase(ext, "htm") || eqIgnoreCase(ext, "shtml")) {
        return "text/html";
    }
    if (eqIgnoreCase(ext, "css")) {
        return "text/css";
    }
    if (eqIgnoreCase(ext, "js") || eqIgnoreCase(ext, "mjs") || eqIgnoreCase(ext, "cjs")) {
        return "text/javascript";
    }
    if (eqIgnoreCase(ext, "ts") || eqIgnoreCase(ext, "mts") || eqIgnoreCase(ext, "cts")) {
        return "text/typescript";
    }
    if (eqIgnoreCase(ext, "jsx")) {
        return "text/jsx";
    }
    if (eqIgnoreCase(ext, "tsx")) {
        return "text/tsx";
    }
    if (eqIgnoreCase(ext, "json") || eqIgnoreCase(ext, "map")) {
        return "application/json";
    }
    if (eqIgnoreCase(ext, "jsonld")) {
        return "application/ld+json";
    }
    if (eqIgnoreCase(ext, "webmanifest")) {
        return "application/manifest+json";
    }
    if (eqIgnoreCase(ext, "xml")) {
        return "application/xml";
    }
    if (eqIgnoreCase(ext, "xhtml")) {
        return "application/xhtml+xml";
    }
    if (eqIgnoreCase(ext, "rss")) {
        return "application/rss+xml";
    }
    if (eqIgnoreCase(ext, "atom")) {
        return "application/atom+xml";
    }
    if (eqIgnoreCase(ext, "txt") || eqIgnoreCase(ext, "text") || eqIgnoreCase(ext, "log")) {
        return "text/plain";
    }
    if (eqIgnoreCase(ext, "md") || eqIgnoreCase(ext, "markdown")) {
        return "text/markdown";
    }
    if (eqIgnoreCase(ext, "csv")) {
        return "text/csv";
    }
    if (eqIgnoreCase(ext, "tsv")) {
        return "text/tab-separated-values";
    }
    if (eqIgnoreCase(ext, "ics")) {
        return "text/calendar";
    }
    if (eqIgnoreCase(ext, "vcf")) {
        return "text/vcard";
    }
    if (eqIgnoreCase(ext, "vtt")) {
        return "text/vtt";
    }

    // Images
    if (eqIgnoreCase(ext, "svg") || eqIgnoreCase(ext, "svgz")) {
        return "image/svg+xml";
    }
    if (eqIgnoreCase(ext, "png")) {
        return "image/png";
    }
    if (eqIgnoreCase(ext, "jpg") || eqIgnoreCase(ext, "jpeg") || eqIgnoreCase(ext, "jpe")) {
        return "image/jpeg";
    }
    if (eqIgnoreCase(ext, "gif")) {
        return "image/gif";
    }
    if (eqIgnoreCase(ext, "webp")) {
        return "image/webp";
    }
    if (eqIgnoreCase(ext, "avif")) {
        return "image/avif";
    }
    if (eqIgnoreCase(ext, "ico")) {
        return "image/x-icon";
    }
    if (eqIgnoreCase(ext, "bmp")) {
        return "image/bmp";
    }
    if (eqIgnoreCase(ext, "tiff") || eqIgnoreCase(ext, "tif")) {
        return "image/tiff";
    }
    if (eqIgnoreCase(ext, "heic")) {
        return "image/heic";
    }
    if (eqIgnoreCase(ext, "heif")) {
        return "image/heif";
    }
    if (eqIgnoreCase(ext, "psd")) {
        return "image/vnd.adobe.photoshop";
    }

    // Documents & Office
    if (eqIgnoreCase(ext, "pdf")) {
        return "application/pdf";
    }
    if (eqIgnoreCase(ext, "rtf")) {
        return "application/rtf";
    }
    if (eqIgnoreCase(ext, "doc")) {
        return "application/msword";
    }
    if (eqIgnoreCase(ext, "docx")) {
        return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    }
    if (eqIgnoreCase(ext, "xls")) {
        return "application/vnd.ms-excel";
    }
    if (eqIgnoreCase(ext, "xlsx")) {
        return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    }
    if (eqIgnoreCase(ext, "ppt")) {
        return "application/vnd.ms-powerpoint";
    }
    if (eqIgnoreCase(ext, "pptx")) {
        return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    }
    if (eqIgnoreCase(ext, "odt")) {
        return "application/vnd.oasis.opendocument.text";
    }
    if (eqIgnoreCase(ext, "ods")) {
        return "application/vnd.oasis.opendocument.spreadsheet";
    }
    if (eqIgnoreCase(ext, "odp")) {
        return "application/vnd.oasis.opendocument.presentation";
    }
    if (eqIgnoreCase(ext, "epub")) {
        return "application/epub+zip";
    }

    // Archives & Binaries
    if (eqIgnoreCase(ext, "zip")) {
        return "application/zip";
    }
    if (eqIgnoreCase(ext, "gz") || eqIgnoreCase(ext, "gzip") || eqIgnoreCase(ext, "tgz")) {
        return "application/gzip";
    }
    if (eqIgnoreCase(ext, "tar")) {
        return "application/x-tar";
    }
    if (eqIgnoreCase(ext, "br")) {
        return "application/x-brotli";
    }
    if (eqIgnoreCase(ext, "zstd") || eqIgnoreCase(ext, "zst")) {
        return "application/zstd";
    }
    if (eqIgnoreCase(ext, "7z")) {
        return "application/x-7z-compressed";
    }
    if (eqIgnoreCase(ext, "rar")) {
        return "application/vnd.rar";
    }
    if (eqIgnoreCase(ext, "bz2")) {
        return "application/x-bzip2";
    }
    if (eqIgnoreCase(ext, "xz")) {
        return "application/x-xz";
    }
    if (eqIgnoreCase(ext, "wasm")) {
        return "application/wasm";
    }
    if (eqIgnoreCase(ext, "bin") || eqIgnoreCase(ext, "dll") || eqIgnoreCase(ext, "so")) {
        return "application/octet-stream";
    }
    if (eqIgnoreCase(ext, "exe")) {
        return "application/x-msdownload";
    }
    if (eqIgnoreCase(ext, "dmg")) {
        return "application/x-apple-diskimage";
    }
    if (eqIgnoreCase(ext, "iso")) {
        return "application/x-iso9660-image";
    }
    if (eqIgnoreCase(ext, "deb")) {
        return "application/x-debian-package";
    }
    if (eqIgnoreCase(ext, "rpm")) {
        return "application/x-redhat-package-manager";
    }
    if (eqIgnoreCase(ext, "apk")) {
        return "application/vnd.android.package-archive";
    }
    if (eqIgnoreCase(ext, "jar")) {
        return "application/java-archive";
    }

    // Audio
    if (eqIgnoreCase(ext, "mp3")) {
        return "audio/mpeg";
    }
    if (eqIgnoreCase(ext, "wav")) {
        return "audio/wav";
    }
    if (eqIgnoreCase(ext, "ogg") || eqIgnoreCase(ext, "oga")) {
        return "audio/ogg";
    }
    if (eqIgnoreCase(ext, "opus")) {
        return "audio/opus";
    }
    if (eqIgnoreCase(ext, "flac")) {
        return "audio/flac";
    }
    if (eqIgnoreCase(ext, "aac")) {
        return "audio/aac";
    }
    if (eqIgnoreCase(ext, "m4a")) {
        return "audio/mp4";
    }
    if (eqIgnoreCase(ext, "weba")) {
        return "audio/webm";
    }
    if (eqIgnoreCase(ext, "mid") || eqIgnoreCase(ext, "midi")) {
        return "audio/midi";
    }

    // Video
    if (eqIgnoreCase(ext, "mp4") || eqIgnoreCase(ext, "m4v")) {
        return "video/mp4";
    }
    if (eqIgnoreCase(ext, "webm")) {
        return "video/webm";
    }
    if (eqIgnoreCase(ext, "mov")) {
        return "video/quicktime";
    }
    if (eqIgnoreCase(ext, "avi")) {
        return "video/x-msvideo";
    }
    if (eqIgnoreCase(ext, "mkv")) {
        return "video/x-matroska";
    }
    if (eqIgnoreCase(ext, "wmv")) {
        return "video/x-ms-wmv";
    }
    if (eqIgnoreCase(ext, "flv")) {
        return "video/x-flv";
    }
    if (eqIgnoreCase(ext, "ogv")) {
        return "video/ogg";
    }
    if (eqIgnoreCase(ext, "m3u8")) {
        return "application/vnd.apple.mpegurl";
    }

    // Fonts
    if (eqIgnoreCase(ext, "woff")) {
        return "font/woff";
    }
    if (eqIgnoreCase(ext, "woff2")) {
        return "font/woff2";
    }
    if (eqIgnoreCase(ext, "ttf")) {
        return "font/ttf";
    }
    if (eqIgnoreCase(ext, "otf")) {
        return "font/otf";
    }
    if (eqIgnoreCase(ext, "eot")) {
        return "application/vnd.ms-fontobject";
    }

    // Config, Source Code & Data
    if (eqIgnoreCase(ext, "yaml") || eqIgnoreCase(ext, "yml")) {
        return "text/yaml";
    }
    if (eqIgnoreCase(ext, "toml")) {
        return "application/toml";
    }
    if (eqIgnoreCase(ext, "ini") || eqIgnoreCase(ext, "conf") || eqIgnoreCase(ext, "env")) {
        return "text/plain";
    }
    if (eqIgnoreCase(ext, "sh") || eqIgnoreCase(ext, "bash")) {
        return "application/x-sh";
    }
    if (eqIgnoreCase(ext, "py")) {
        return "text/x-python";
    }
    if (eqIgnoreCase(ext, "rs")) {
        return "text/x-rust";
    }
    if (eqIgnoreCase(ext, "cp")) {
        return "text/x-cp";
    }
    if (eqIgnoreCase(ext, "zig")) {
        return "text/x-zig";
    }
    if (eqIgnoreCase(ext, "c") || eqIgnoreCase(ext, "h")) {
        return "text/x-c";
    }
    if (eqIgnoreCase(ext, "cpp") || eqIgnoreCase(ext, "cc") || eqIgnoreCase(ext, "hpp")) {
        return "text/x-c++";
    }
    if (eqIgnoreCase(ext, "go")) {
        return "text/x-go";
    }
    if (eqIgnoreCase(ext, "java")) {
        return "text/x-java-source";
    }
    if (eqIgnoreCase(ext, "rb")) {
        return "text/x-ruby";
    }
    if (eqIgnoreCase(ext, "php")) {
        return "application/x-httpd-php";
    }
    if (eqIgnoreCase(ext, "sql")) {
        return "application/sql";
    }
    if (eqIgnoreCase(ext, "graphql") || eqIgnoreCase(ext, "gql")) {
        return "application/graphql";
    }
    if (eqIgnoreCase(ext, "proto")) {
        return "text/x-protobuf";
    }
    if (eqIgnoreCase(ext, "pem") || eqIgnoreCase(ext, "crt") || eqIgnoreCase(ext, "cer")) {
        return "application/x-x509-ca-cert";
    }
    if (eqIgnoreCase(ext, "der")) {
        return "application/x-x509-ca-cert";
    }

    return null;
}

fn mtypExtensionForEssence(essence: string): string? {
    // Web & Markup
    if (eqIgnoreCase(essence, "text/html")) { return "html"; }
    if (eqIgnoreCase(essence, "text/css")) { return "css"; }
    if (eqIgnoreCase(essence, "text/javascript") || eqIgnoreCase(essence, "application/javascript") || eqIgnoreCase(essence, "application/ecmascript")) { return "js"; }
    if (eqIgnoreCase(essence, "text/typescript") || eqIgnoreCase(essence, "application/typescript")) { return "ts"; }
    if (eqIgnoreCase(essence, "text/jsx")) { return "jsx"; }
    if (eqIgnoreCase(essence, "text/tsx")) { return "tsx"; }
    if (eqIgnoreCase(essence, "application/json")) { return "json"; }
    if (eqIgnoreCase(essence, "application/ld+json")) { return "jsonld"; }
    if (eqIgnoreCase(essence, "application/manifest+json")) { return "webmanifest"; }
    if (eqIgnoreCase(essence, "application/xml") || eqIgnoreCase(essence, "text/xml")) { return "xml"; }
    if (eqIgnoreCase(essence, "application/xhtml+xml")) { return "xhtml"; }
    if (eqIgnoreCase(essence, "application/rss+xml")) { return "rss"; }
    if (eqIgnoreCase(essence, "application/atom+xml")) { return "atom"; }
    if (eqIgnoreCase(essence, "text/plain")) { return "txt"; }
    if (eqIgnoreCase(essence, "text/markdown")) { return "md"; }
    if (eqIgnoreCase(essence, "text/csv")) { return "csv"; }
    if (eqIgnoreCase(essence, "text/tab-separated-values")) { return "tsv"; }
    if (eqIgnoreCase(essence, "text/calendar")) { return "ics"; }
    if (eqIgnoreCase(essence, "text/vcard")) { return "vcf"; }
    if (eqIgnoreCase(essence, "text/vtt")) { return "vtt"; }

    // Images
    if (eqIgnoreCase(essence, "image/svg+xml")) { return "svg"; }
    if (eqIgnoreCase(essence, "image/png")) { return "png"; }
    if (eqIgnoreCase(essence, "image/jpeg")) { return "jpeg"; }
    if (eqIgnoreCase(essence, "image/gif")) { return "gif"; }
    if (eqIgnoreCase(essence, "image/webp")) { return "webp"; }
    if (eqIgnoreCase(essence, "image/avif")) { return "avif"; }
    if (eqIgnoreCase(essence, "image/x-icon") || eqIgnoreCase(essence, "image/vnd.microsoft.icon")) { return "ico"; }
    if (eqIgnoreCase(essence, "image/bmp")) { return "bmp"; }
    if (eqIgnoreCase(essence, "image/tiff")) { return "tiff"; }
    if (eqIgnoreCase(essence, "image/heic")) { return "heic"; }
    if (eqIgnoreCase(essence, "image/heif")) { return "heif"; }
    if (eqIgnoreCase(essence, "image/vnd.adobe.photoshop")) { return "psd"; }

    // Documents & Office
    if (eqIgnoreCase(essence, "application/pdf")) { return "pdf"; }
    if (eqIgnoreCase(essence, "application/rtf")) { return "rtf"; }
    if (eqIgnoreCase(essence, "application/msword")) { return "doc"; }
    if (eqIgnoreCase(essence, "application/vnd.openxmlformats-officedocument.wordprocessingml.document")) { return "docx"; }
    if (eqIgnoreCase(essence, "application/vnd.ms-excel")) { return "xls"; }
    if (eqIgnoreCase(essence, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet")) { return "xlsx"; }
    if (eqIgnoreCase(essence, "application/vnd.ms-powerpoint")) { return "ppt"; }
    if (eqIgnoreCase(essence, "application/vnd.openxmlformats-officedocument.presentationml.presentation")) { return "pptx"; }
    if (eqIgnoreCase(essence, "application/vnd.oasis.opendocument.text")) { return "odt"; }
    if (eqIgnoreCase(essence, "application/vnd.oasis.opendocument.spreadsheet")) { return "ods"; }
    if (eqIgnoreCase(essence, "application/vnd.oasis.opendocument.presentation")) { return "odp"; }
    if (eqIgnoreCase(essence, "application/epub+zip")) { return "epub"; }

    // Archives & Binaries
    if (eqIgnoreCase(essence, "application/zip")) { return "zip"; }
    if (eqIgnoreCase(essence, "application/gzip")) { return "gz"; }
    if (eqIgnoreCase(essence, "application/x-tar")) { return "tar"; }
    if (eqIgnoreCase(essence, "application/x-brotli")) { return "br"; }
    if (eqIgnoreCase(essence, "application/zstd")) { return "zstd"; }
    if (eqIgnoreCase(essence, "application/x-7z-compressed")) { return "7z"; }
    if (eqIgnoreCase(essence, "application/vnd.rar")) { return "rar"; }
    if (eqIgnoreCase(essence, "application/x-bzip2")) { return "bz2"; }
    if (eqIgnoreCase(essence, "application/x-xz")) { return "xz"; }
    if (eqIgnoreCase(essence, "application/wasm")) { return "wasm"; }
    if (eqIgnoreCase(essence, "application/octet-stream")) { return "bin"; }
    if (eqIgnoreCase(essence, "application/x-msdownload")) { return "exe"; }
    if (eqIgnoreCase(essence, "application/x-apple-diskimage")) { return "dmg"; }
    if (eqIgnoreCase(essence, "application/x-iso9660-image")) { return "iso"; }
    if (eqIgnoreCase(essence, "application/x-debian-package")) { return "deb"; }
    if (eqIgnoreCase(essence, "application/x-redhat-package-manager")) { return "rpm"; }
    if (eqIgnoreCase(essence, "application/vnd.android.package-archive")) { return "apk"; }
    if (eqIgnoreCase(essence, "application/java-archive")) { return "jar"; }

    // Audio
    if (eqIgnoreCase(essence, "audio/mpeg")) { return "mp3"; }
    if (eqIgnoreCase(essence, "audio/wav")) { return "wav"; }
    if (eqIgnoreCase(essence, "audio/ogg")) { return "ogg"; }
    if (eqIgnoreCase(essence, "audio/opus")) { return "opus"; }
    if (eqIgnoreCase(essence, "audio/flac")) { return "flac"; }
    if (eqIgnoreCase(essence, "audio/aac")) { return "aac"; }
    if (eqIgnoreCase(essence, "audio/mp4")) { return "m4a"; }
    if (eqIgnoreCase(essence, "audio/webm")) { return "weba"; }
    if (eqIgnoreCase(essence, "audio/midi")) { return "midi"; }

    // Video
    if (eqIgnoreCase(essence, "video/mp4")) { return "mp4"; }
    if (eqIgnoreCase(essence, "video/webm")) { return "webm"; }
    if (eqIgnoreCase(essence, "video/quicktime")) { return "mov"; }
    if (eqIgnoreCase(essence, "video/x-msvideo")) { return "avi"; }
    if (eqIgnoreCase(essence, "video/x-matroska")) { return "mkv"; }
    if (eqIgnoreCase(essence, "video/x-ms-wmv")) { return "wmv"; }
    if (eqIgnoreCase(essence, "video/x-flv")) { return "flv"; }
    if (eqIgnoreCase(essence, "video/ogg")) { return "ogv"; }
    if (eqIgnoreCase(essence, "application/vnd.apple.mpegurl")) { return "m3u8"; }

    // Fonts
    if (eqIgnoreCase(essence, "font/woff")) { return "woff"; }
    if (eqIgnoreCase(essence, "font/woff2")) { return "woff2"; }
    if (eqIgnoreCase(essence, "font/ttf")) { return "ttf"; }
    if (eqIgnoreCase(essence, "font/otf")) { return "otf"; }
    if (eqIgnoreCase(essence, "application/vnd.ms-fontobject")) { return "eot"; }

    // Config & Code
    if (eqIgnoreCase(essence, "text/yaml") || eqIgnoreCase(essence, "application/yaml") || eqIgnoreCase(essence, "application/x-yaml")) { return "yaml"; }
    if (eqIgnoreCase(essence, "application/toml")) { return "toml"; }
    if (eqIgnoreCase(essence, "application/x-sh")) { return "sh"; }
    if (eqIgnoreCase(essence, "text/x-python")) { return "py"; }
    if (eqIgnoreCase(essence, "text/x-rust")) { return "rs"; }
    if (eqIgnoreCase(essence, "text/x-cp")) { return "cp"; }
    if (eqIgnoreCase(essence, "text/x-zig")) { return "zig"; }
    if (eqIgnoreCase(essence, "text/x-c")) { return "c"; }
    if (eqIgnoreCase(essence, "text/x-c++")) { return "cpp"; }
    if (eqIgnoreCase(essence, "text/x-go")) { return "go"; }
    if (eqIgnoreCase(essence, "text/x-java-source")) { return "java"; }
    if (eqIgnoreCase(essence, "text/x-ruby")) { return "rb"; }
    if (eqIgnoreCase(essence, "application/x-httpd-php")) { return "php"; }
    if (eqIgnoreCase(essence, "application/sql")) { return "sql"; }
    if (eqIgnoreCase(essence, "application/graphql")) { return "graphql"; }
    if (eqIgnoreCase(essence, "text/x-protobuf")) { return "proto"; }
    if (eqIgnoreCase(essence, "application/x-x509-ca-cert")) { return "pem"; }

    return null;
}

// ============================================================================
// Public API (npm `mime-types` v2.1 parity)
// ============================================================================

/// Parse a raw MIME / Content-Type header string into its structural parts:
/// `essence` (e.g. `"text/html"`), `type_part` (`"text"`), `subtype` (`"html"`),
/// and `charset_val` (e.g. `"utf-8"` if a `charset=` parameter is present, else `""`).
pub fn parseMimeType(raw: string): MimeParts {
    const empty = MimeParts {
        valid: false,
        essence: "",
        type_part: "",
        subtype: "",
        charset_val: "",
    };
    const s: string = trim(raw);
    if (s.len == 0) {
        return empty;
    }

    // Split at first ';' to separate essence from parameters
    const semi: usize = findByte(s, 59, 0) ?? s.len;
    const essence: string = trim(s[0..semi]);
    if (essence.len == 0) {
        return empty;
    }

    // Find single '/' separating type and subtype
    let slash: i64 = -1;
    let i: usize = 0;
    while (i < essence.len) {
        if (essence[i] == 47) {
            if (slash >= 0) {
                return empty;
            }
            slash = i as i64;
        }
        i = i + 1;
    }
    if (slash <= 0) {
        return empty;
    }
    const slash_idx: usize = slash as usize;
    if (slash_idx + 1 >= essence.len) {
        return empty;
    }
    const type_part: string = essence[0..slash_idx];
    const subtype: string = essence[(slash_idx + 1)..essence.len];
    if (!mtypIsValidToken(type_part) || !mtypIsValidToken(subtype)) {
        return empty;
    }

    // Parse parameters for `charset=...`
    let charset_val: string = "";
    let p: usize = semi;
    while (p < s.len) {
        if (s[p] == 59) {
            p = p + 1;
        }
        const next_semi: usize = findByte(s, 59, p) ?? s.len;
        const seg: string = s[p..next_semi];
        if (eq_idx := findByte(seg, 61, 0)) {
            if (eq_idx > 0) {
                const key: string = trim(seg[0..eq_idx]);
                if (eqIgnoreCase(key, "charset")) {
                    const raw_val: string = trim(seg[(eq_idx + 1)..seg.len]);
                    if (raw_val.len >= 2 && ((raw_val[0] == 34 && raw_val[raw_val.len - 1] == 34) ||
                        (raw_val[0] == 39 && raw_val[raw_val.len - 1] == 39))) {
                        charset_val = raw_val[1..(raw_val.len - 1)];
                    } else {
                        charset_val = raw_val;
                    }
                }
            }
        }
        p = next_semi;
    }

    return MimeParts {
        valid: true,
        essence: essence,
        type_part: type_part,
        subtype: subtype,
        charset_val: charset_val,
    };
}

/// Look up the MIME type for a file path, dotted extension (`.html`), or bare extension (`png`).
/// Case-insensitive. Returns `null` if unknown.
pub fn lookup(path_or_ext: string): string? {
    const ext_opt = mtypExtractExt(path_or_ext);
    if (ext_opt == null) {
        return null;
    }
    const ext: string = ext_opt ?? "";
    return mtypLookupExt(ext);
}

/// Return the default file extension (without leading dot) for a MIME type or Content-Type header.
/// Returns `null` if unknown or invalid.
pub fn extension(mime_type: string): string? {
    const parts = parseMimeType(mime_type);
    if (!parts.valid) {
        return null;
    }
    return mtypExtensionForEssence(parts.essence);
}

/// Return the default charset (`"UTF-8"`) for textual and structured-text MIME types
/// (`text/*`, `application/json`, `application/javascript`, `application/xml`,
/// `image/svg+xml`, `*/*+json`, `*/*+xml`), or `null` for binary types.
pub fn charset(mime_type: string): string? {
    const parts = parseMimeType(mime_type);
    if (!parts.valid) {
        return null;
    }
    if (eqIgnoreCase(parts.type_part, "text")) {
        return "UTF-8";
    }
    if (eqIgnoreCase(parts.essence, "application/json") ||
        eqIgnoreCase(parts.essence, "application/javascript") ||
        eqIgnoreCase(parts.essence, "application/ecmascript") ||
        eqIgnoreCase(parts.essence, "application/xml") ||
        eqIgnoreCase(parts.essence, "application/xhtml+xml") ||
        eqIgnoreCase(parts.essence, "application/graphql") ||
        eqIgnoreCase(parts.essence, "application/sql") ||
        eqIgnoreCase(parts.essence, "application/toml") ||
        eqIgnoreCase(parts.essence, "application/yaml") ||
        eqIgnoreCase(parts.essence, "application/x-yaml") ||
        eqIgnoreCase(parts.essence, "application/x-www-form-urlencoded") ||
        eqIgnoreCase(parts.essence, "image/svg+xml")) {
        return "UTF-8";
    }
    if (mtypEndsWithIgnoreCase(parts.subtype, "+json") ||
        mtypEndsWithIgnoreCase(parts.subtype, "+xml")) {
        return "UTF-8";
    }
    return null;
}

fn mtypIsTopLevelType(t: string): bool {
    return eqIgnoreCase(t, "text") ||
        eqIgnoreCase(t, "image") ||
        eqIgnoreCase(t, "audio") ||
        eqIgnoreCase(t, "video") ||
        eqIgnoreCase(t, "font") ||
        eqIgnoreCase(t, "application") ||
        eqIgnoreCase(t, "multipart") ||
        eqIgnoreCase(t, "message") ||
        eqIgnoreCase(t, "model") ||
        eqIgnoreCase(t, "example") ||
        (t.len >= 2 && (t[0] == 120 || t[0] == 88) && t[1] == 45);
}

/// Build a full HTTP `Content-Type` header value in caller-provided `out` from either
/// a file path / extension (e.g. `"html"`, `"/a/b.json"`) or a MIME type (`"text/plain"`).
/// Automatically appends `; charset=utf-8` when appropriate and not already present.
/// Returns the number of bytes written (0 if unknown or invalid).
pub fn contentType(type_or_ext: string, out: []u8): usize {
    if (type_or_ext.len == 0 || out.len == 0) {
        return 0;
    }
    const trimmed: string = trim(type_or_ext);
    if (trimmed.len == 0) {
        return 0;
    }

    let base_mime: string = "";
    let has_charset: bool = false;

    const parsed = parseMimeType(trimmed);
    if (parsed.valid && mtypIsTopLevelType(parsed.type_part)) {
        base_mime = trimmed;
        has_charset = parsed.charset_val.len > 0;
    } else {
        const looked = lookup(trimmed);
        if (looked == null) {
            return 0;
        }
        base_mime = looked ?? "";
    }

    let pos: usize = writeStr(base_mime, out, 0);
    if (!has_charset) {
        const cs = charset(base_mime);
        if (cs != null) {
            pos = writeStr("; charset=utf-8", out, pos);
        }
    }
    return pos;
}

/// Return true if responses with `mime_type` benefit from HTTP compression (gzip/brotli/zstd).
/// Returns false for already-compressed formats (png, jpeg, webp, avif, zip, gz, mp4, woff2, etc.).
pub fn isCompressible(mime_type: string): bool {
    const parts = parseMimeType(mime_type);
    if (!parts.valid) {
        return false;
    }
    if (eqIgnoreCase(parts.type_part, "text")) {
        return true;
    }
    if (mtypEndsWithIgnoreCase(parts.subtype, "+json") ||
        mtypEndsWithIgnoreCase(parts.subtype, "+xml") ||
        mtypEndsWithIgnoreCase(parts.subtype, "+text")) {
        return true;
    }
    if (eqIgnoreCase(parts.essence, "application/json") ||
        eqIgnoreCase(parts.essence, "application/javascript") ||
        eqIgnoreCase(parts.essence, "application/ecmascript") ||
        eqIgnoreCase(parts.essence, "application/xml") ||
        eqIgnoreCase(parts.essence, "application/wasm") ||
        eqIgnoreCase(parts.essence, "application/graphql") ||
        eqIgnoreCase(parts.essence, "application/sql") ||
        eqIgnoreCase(parts.essence, "application/toml") ||
        eqIgnoreCase(parts.essence, "application/yaml") ||
        eqIgnoreCase(parts.essence, "application/x-yaml") ||
        eqIgnoreCase(parts.essence, "application/rtf") ||
        eqIgnoreCase(parts.essence, "application/x-sh") ||
        eqIgnoreCase(parts.essence, "application/x-www-form-urlencoded") ||
        eqIgnoreCase(parts.essence, "application/vnd.apple.mpegurl") ||
        eqIgnoreCase(parts.essence, "application/vnd.ms-fontobject") ||
        eqIgnoreCase(parts.essence, "image/svg+xml") ||
        eqIgnoreCase(parts.essence, "image/bmp") ||
        eqIgnoreCase(parts.essence, "image/x-icon") ||
        eqIgnoreCase(parts.essence, "image/vnd.microsoft.icon") ||
        eqIgnoreCase(parts.essence, "font/ttf") ||
        eqIgnoreCase(parts.essence, "font/otf")) {
        return true;
    }
    return false;
}
