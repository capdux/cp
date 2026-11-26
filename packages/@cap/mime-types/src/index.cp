// @cap/mime-types — Zero-heap MIME type / extension / charset / Content-Type resolver in pure CP
// Clean-room MIT implementation inspired by npm `mime-types` v2.1.

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

fn mtypToLower(c: u8): u8 {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }
    return c;
}

fn mtypIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn mtypIsTokenChar(c: u8): bool {
    if ((c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
        return true;
    }
    return c == 33 || c == 35 || c == 36 || c == 38 || c == 43 || c == 45 ||
        c == 46 || c == 94 || c == 95 || c == 96 || c == 124 || c == 126;
}

fn mtypEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (mtypToLower(a[i]) != mtypToLower(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn mtypEndsWithIgnoreCase(s: string, suffix: string): bool {
    if (s.len < suffix.len) {
        return false;
    }
    const offset: usize = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (mtypToLower(s[offset + i]) != mtypToLower(suffix[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn mtypContainsSlash(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 47) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn mtypTrimBounds(s: string, out_start: *usize, out_end: *usize) {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && mtypIsSpace(s[a])) {
        a = a + 1;
    }
    while (b > a && mtypIsSpace(s[b - 1])) {
        b = b - 1;
    }
    out_start.* = a;
    out_end.* = b;
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

fn mtypWriteStr(out: []u8, pos: *usize, s: string) {
    let i: usize = 0;
    while (i < s.len && pos.* < out.len) {
        out[pos.*] = s[i];
        pos.* = pos.* + 1;
        i = i + 1;
    }
}

fn mtypExtractExt(path_or_ext: string): string? {
    let ts: usize = 0;
    let te: usize = 0;
    mtypTrimBounds(path_or_ext, &ts, &te);
    if (ts >= te) {
        return null;
    }
    const s: string = path_or_ext[ts..te];

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
    let last_dot: i64 = -1;
    let j: usize = 0;
    while (j < base.len) {
        if (base[j] == 46) {
            last_dot = j as i64;
        }
        j = j + 1;
    }
    if (last_dot < 0) {
        if (has_sep) {
            return null;
        }
        return base;
    }
    const dot_idx: usize = last_dot as usize;
    if (dot_idx + 1 >= base.len) {
        return null;
    }
    return base[(dot_idx + 1)..base.len];
}

// ============================================================================
// MIME Type Database (95+ Web / Dev / Media / Font / Document / Archive types)
// ============================================================================

fn mtypLookupExt(ext: string): string? {
    // Web & Markup
    if (mtypEqIgnoreCase(ext, "html") || mtypEqIgnoreCase(ext, "htm") || mtypEqIgnoreCase(ext, "shtml")) {
        return "text/html";
    }
    if (mtypEqIgnoreCase(ext, "css")) {
        return "text/css";
    }
    if (mtypEqIgnoreCase(ext, "js") || mtypEqIgnoreCase(ext, "mjs") || mtypEqIgnoreCase(ext, "cjs")) {
        return "text/javascript";
    }
    if (mtypEqIgnoreCase(ext, "ts") || mtypEqIgnoreCase(ext, "mts") || mtypEqIgnoreCase(ext, "cts")) {
        return "text/typescript";
    }
    if (mtypEqIgnoreCase(ext, "jsx")) {
        return "text/jsx";
    }
    if (mtypEqIgnoreCase(ext, "tsx")) {
        return "text/tsx";
    }
    if (mtypEqIgnoreCase(ext, "json") || mtypEqIgnoreCase(ext, "map")) {
        return "application/json";
    }
    if (mtypEqIgnoreCase(ext, "jsonld")) {
        return "application/ld+json";
    }
    if (mtypEqIgnoreCase(ext, "webmanifest")) {
        return "application/manifest+json";
    }
    if (mtypEqIgnoreCase(ext, "xml")) {
        return "application/xml";
    }
    if (mtypEqIgnoreCase(ext, "xhtml")) {
        return "application/xhtml+xml";
    }
    if (mtypEqIgnoreCase(ext, "rss")) {
        return "application/rss+xml";
    }
    if (mtypEqIgnoreCase(ext, "atom")) {
        return "application/atom+xml";
    }
    if (mtypEqIgnoreCase(ext, "txt") || mtypEqIgnoreCase(ext, "text") || mtypEqIgnoreCase(ext, "log")) {
        return "text/plain";
    }
    if (mtypEqIgnoreCase(ext, "md") || mtypEqIgnoreCase(ext, "markdown")) {
        return "text/markdown";
    }
    if (mtypEqIgnoreCase(ext, "csv")) {
        return "text/csv";
    }
    if (mtypEqIgnoreCase(ext, "tsv")) {
        return "text/tab-separated-values";
    }
    if (mtypEqIgnoreCase(ext, "ics")) {
        return "text/calendar";
    }
    if (mtypEqIgnoreCase(ext, "vcf")) {
        return "text/vcard";
    }
    if (mtypEqIgnoreCase(ext, "vtt")) {
        return "text/vtt";
    }

    // Images
    if (mtypEqIgnoreCase(ext, "svg") || mtypEqIgnoreCase(ext, "svgz")) {
        return "image/svg+xml";
    }
    if (mtypEqIgnoreCase(ext, "png")) {
        return "image/png";
    }
    if (mtypEqIgnoreCase(ext, "jpg") || mtypEqIgnoreCase(ext, "jpeg") || mtypEqIgnoreCase(ext, "jpe")) {
        return "image/jpeg";
    }
    if (mtypEqIgnoreCase(ext, "gif")) {
        return "image/gif";
    }
    if (mtypEqIgnoreCase(ext, "webp")) {
        return "image/webp";
    }
    if (mtypEqIgnoreCase(ext, "avif")) {
        return "image/avif";
    }
    if (mtypEqIgnoreCase(ext, "ico")) {
        return "image/x-icon";
    }
    if (mtypEqIgnoreCase(ext, "bmp")) {
        return "image/bmp";
    }
    if (mtypEqIgnoreCase(ext, "tiff") || mtypEqIgnoreCase(ext, "tif")) {
        return "image/tiff";
    }
    if (mtypEqIgnoreCase(ext, "heic")) {
        return "image/heic";
    }
    if (mtypEqIgnoreCase(ext, "heif")) {
        return "image/heif";
    }
    if (mtypEqIgnoreCase(ext, "psd")) {
        return "image/vnd.adobe.photoshop";
    }

    // Documents & Office
    if (mtypEqIgnoreCase(ext, "pdf")) {
        return "application/pdf";
    }
    if (mtypEqIgnoreCase(ext, "rtf")) {
        return "application/rtf";
    }
    if (mtypEqIgnoreCase(ext, "doc")) {
        return "application/msword";
    }
    if (mtypEqIgnoreCase(ext, "docx")) {
        return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    }
    if (mtypEqIgnoreCase(ext, "xls")) {
        return "application/vnd.ms-excel";
    }
    if (mtypEqIgnoreCase(ext, "xlsx")) {
        return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    }
    if (mtypEqIgnoreCase(ext, "ppt")) {
        return "application/vnd.ms-powerpoint";
    }
    if (mtypEqIgnoreCase(ext, "pptx")) {
        return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    }
    if (mtypEqIgnoreCase(ext, "odt")) {
        return "application/vnd.oasis.opendocument.text";
    }
    if (mtypEqIgnoreCase(ext, "ods")) {
        return "application/vnd.oasis.opendocument.spreadsheet";
    }
    if (mtypEqIgnoreCase(ext, "odp")) {
        return "application/vnd.oasis.opendocument.presentation";
    }
    if (mtypEqIgnoreCase(ext, "epub")) {
        return "application/epub+zip";
    }

    // Archives & Binaries
    if (mtypEqIgnoreCase(ext, "zip")) {
        return "application/zip";
    }
    if (mtypEqIgnoreCase(ext, "gz") || mtypEqIgnoreCase(ext, "gzip") || mtypEqIgnoreCase(ext, "tgz")) {
        return "application/gzip";
    }
    if (mtypEqIgnoreCase(ext, "tar")) {
        return "application/x-tar";
    }
    if (mtypEqIgnoreCase(ext, "br")) {
        return "application/x-brotli";
    }
    if (mtypEqIgnoreCase(ext, "zstd") || mtypEqIgnoreCase(ext, "zst")) {
        return "application/zstd";
    }
    if (mtypEqIgnoreCase(ext, "7z")) {
        return "application/x-7z-compressed";
    }
    if (mtypEqIgnoreCase(ext, "rar")) {
        return "application/vnd.rar";
    }
    if (mtypEqIgnoreCase(ext, "bz2")) {
        return "application/x-bzip2";
    }
    if (mtypEqIgnoreCase(ext, "xz")) {
        return "application/x-xz";
    }
    if (mtypEqIgnoreCase(ext, "wasm")) {
        return "application/wasm";
    }
    if (mtypEqIgnoreCase(ext, "bin") || mtypEqIgnoreCase(ext, "dll") || mtypEqIgnoreCase(ext, "so")) {
        return "application/octet-stream";
    }
    if (mtypEqIgnoreCase(ext, "exe")) {
        return "application/x-msdownload";
    }
    if (mtypEqIgnoreCase(ext, "dmg")) {
        return "application/x-apple-diskimage";
    }
    if (mtypEqIgnoreCase(ext, "iso")) {
        return "application/x-iso9660-image";
    }
    if (mtypEqIgnoreCase(ext, "deb")) {
        return "application/x-debian-package";
    }
    if (mtypEqIgnoreCase(ext, "rpm")) {
        return "application/x-redhat-package-manager";
    }
    if (mtypEqIgnoreCase(ext, "apk")) {
        return "application/vnd.android.package-archive";
    }
    if (mtypEqIgnoreCase(ext, "jar")) {
        return "application/java-archive";
    }

    // Audio
    if (mtypEqIgnoreCase(ext, "mp3")) {
        return "audio/mpeg";
    }
    if (mtypEqIgnoreCase(ext, "wav")) {
        return "audio/wav";
    }
    if (mtypEqIgnoreCase(ext, "ogg") || mtypEqIgnoreCase(ext, "oga")) {
        return "audio/ogg";
    }
    if (mtypEqIgnoreCase(ext, "opus")) {
        return "audio/opus";
    }
    if (mtypEqIgnoreCase(ext, "flac")) {
        return "audio/flac";
    }
    if (mtypEqIgnoreCase(ext, "aac")) {
        return "audio/aac";
    }
    if (mtypEqIgnoreCase(ext, "m4a")) {
        return "audio/mp4";
    }
    if (mtypEqIgnoreCase(ext, "weba")) {
        return "audio/webm";
    }
    if (mtypEqIgnoreCase(ext, "mid") || mtypEqIgnoreCase(ext, "midi")) {
        return "audio/midi";
    }

    // Video
    if (mtypEqIgnoreCase(ext, "mp4") || mtypEqIgnoreCase(ext, "m4v")) {
        return "video/mp4";
    }
    if (mtypEqIgnoreCase(ext, "webm")) {
        return "video/webm";
    }
    if (mtypEqIgnoreCase(ext, "mov")) {
        return "video/quicktime";
    }
    if (mtypEqIgnoreCase(ext, "avi")) {
        return "video/x-msvideo";
    }
    if (mtypEqIgnoreCase(ext, "mkv")) {
        return "video/x-matroska";
    }
    if (mtypEqIgnoreCase(ext, "wmv")) {
        return "video/x-ms-wmv";
    }
    if (mtypEqIgnoreCase(ext, "flv")) {
        return "video/x-flv";
    }
    if (mtypEqIgnoreCase(ext, "ogv")) {
        return "video/ogg";
    }
    if (mtypEqIgnoreCase(ext, "m3u8")) {
        return "application/vnd.apple.mpegurl";
    }

    // Fonts
    if (mtypEqIgnoreCase(ext, "woff")) {
        return "font/woff";
    }
    if (mtypEqIgnoreCase(ext, "woff2")) {
        return "font/woff2";
    }
    if (mtypEqIgnoreCase(ext, "ttf")) {
        return "font/ttf";
    }
    if (mtypEqIgnoreCase(ext, "otf")) {
        return "font/otf";
    }
    if (mtypEqIgnoreCase(ext, "eot")) {
        return "application/vnd.ms-fontobject";
    }

    // Config, Source Code & Data
    if (mtypEqIgnoreCase(ext, "yaml") || mtypEqIgnoreCase(ext, "yml")) {
        return "text/yaml";
    }
    if (mtypEqIgnoreCase(ext, "toml")) {
        return "application/toml";
    }
    if (mtypEqIgnoreCase(ext, "ini") || mtypEqIgnoreCase(ext, "conf") || mtypEqIgnoreCase(ext, "env")) {
        return "text/plain";
    }
    if (mtypEqIgnoreCase(ext, "sh") || mtypEqIgnoreCase(ext, "bash")) {
        return "application/x-sh";
    }
    if (mtypEqIgnoreCase(ext, "py")) {
        return "text/x-python";
    }
    if (mtypEqIgnoreCase(ext, "rs")) {
        return "text/x-rust";
    }
    if (mtypEqIgnoreCase(ext, "cp")) {
        return "text/x-cp";
    }
    if (mtypEqIgnoreCase(ext, "zig")) {
        return "text/x-zig";
    }
    if (mtypEqIgnoreCase(ext, "c") || mtypEqIgnoreCase(ext, "h")) {
        return "text/x-c";
    }
    if (mtypEqIgnoreCase(ext, "cpp") || mtypEqIgnoreCase(ext, "cc") || mtypEqIgnoreCase(ext, "hpp")) {
        return "text/x-c++";
    }
    if (mtypEqIgnoreCase(ext, "go")) {
        return "text/x-go";
    }
    if (mtypEqIgnoreCase(ext, "java")) {
        return "text/x-java-source";
    }
    if (mtypEqIgnoreCase(ext, "rb")) {
        return "text/x-ruby";
    }
    if (mtypEqIgnoreCase(ext, "php")) {
        return "application/x-httpd-php";
    }
    if (mtypEqIgnoreCase(ext, "sql")) {
        return "application/sql";
    }
    if (mtypEqIgnoreCase(ext, "graphql") || mtypEqIgnoreCase(ext, "gql")) {
        return "application/graphql";
    }
    if (mtypEqIgnoreCase(ext, "proto")) {
        return "text/x-protobuf";
    }
    if (mtypEqIgnoreCase(ext, "pem") || mtypEqIgnoreCase(ext, "crt") || mtypEqIgnoreCase(ext, "cer")) {
        return "application/x-x509-ca-cert";
    }
    if (mtypEqIgnoreCase(ext, "der")) {
        return "application/x-x509-ca-cert";
    }

    return null;
}

fn mtypExtensionForEssence(essence: string): string? {
    // Web & Markup
    if (mtypEqIgnoreCase(essence, "text/html")) { return "html"; }
    if (mtypEqIgnoreCase(essence, "text/css")) { return "css"; }
    if (mtypEqIgnoreCase(essence, "text/javascript") || mtypEqIgnoreCase(essence, "application/javascript") || mtypEqIgnoreCase(essence, "application/ecmascript")) { return "js"; }
    if (mtypEqIgnoreCase(essence, "text/typescript") || mtypEqIgnoreCase(essence, "application/typescript")) { return "ts"; }
    if (mtypEqIgnoreCase(essence, "text/jsx")) { return "jsx"; }
    if (mtypEqIgnoreCase(essence, "text/tsx")) { return "tsx"; }
    if (mtypEqIgnoreCase(essence, "application/json")) { return "json"; }
    if (mtypEqIgnoreCase(essence, "application/ld+json")) { return "jsonld"; }
    if (mtypEqIgnoreCase(essence, "application/manifest+json")) { return "webmanifest"; }
    if (mtypEqIgnoreCase(essence, "application/xml") || mtypEqIgnoreCase(essence, "text/xml")) { return "xml"; }
    if (mtypEqIgnoreCase(essence, "application/xhtml+xml")) { return "xhtml"; }
    if (mtypEqIgnoreCase(essence, "application/rss+xml")) { return "rss"; }
    if (mtypEqIgnoreCase(essence, "application/atom+xml")) { return "atom"; }
    if (mtypEqIgnoreCase(essence, "text/plain")) { return "txt"; }
    if (mtypEqIgnoreCase(essence, "text/markdown")) { return "md"; }
    if (mtypEqIgnoreCase(essence, "text/csv")) { return "csv"; }
    if (mtypEqIgnoreCase(essence, "text/tab-separated-values")) { return "tsv"; }
    if (mtypEqIgnoreCase(essence, "text/calendar")) { return "ics"; }
    if (mtypEqIgnoreCase(essence, "text/vcard")) { return "vcf"; }
    if (mtypEqIgnoreCase(essence, "text/vtt")) { return "vtt"; }

    // Images
    if (mtypEqIgnoreCase(essence, "image/svg+xml")) { return "svg"; }
    if (mtypEqIgnoreCase(essence, "image/png")) { return "png"; }
    if (mtypEqIgnoreCase(essence, "image/jpeg")) { return "jpeg"; }
    if (mtypEqIgnoreCase(essence, "image/gif")) { return "gif"; }
    if (mtypEqIgnoreCase(essence, "image/webp")) { return "webp"; }
    if (mtypEqIgnoreCase(essence, "image/avif")) { return "avif"; }
    if (mtypEqIgnoreCase(essence, "image/x-icon") || mtypEqIgnoreCase(essence, "image/vnd.microsoft.icon")) { return "ico"; }
    if (mtypEqIgnoreCase(essence, "image/bmp")) { return "bmp"; }
    if (mtypEqIgnoreCase(essence, "image/tiff")) { return "tiff"; }
    if (mtypEqIgnoreCase(essence, "image/heic")) { return "heic"; }
    if (mtypEqIgnoreCase(essence, "image/heif")) { return "heif"; }
    if (mtypEqIgnoreCase(essence, "image/vnd.adobe.photoshop")) { return "psd"; }

    // Documents & Office
    if (mtypEqIgnoreCase(essence, "application/pdf")) { return "pdf"; }
    if (mtypEqIgnoreCase(essence, "application/rtf")) { return "rtf"; }
    if (mtypEqIgnoreCase(essence, "application/msword")) { return "doc"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.openxmlformats-officedocument.wordprocessingml.document")) { return "docx"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.ms-excel")) { return "xls"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet")) { return "xlsx"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.ms-powerpoint")) { return "ppt"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.openxmlformats-officedocument.presentationml.presentation")) { return "pptx"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.oasis.opendocument.text")) { return "odt"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.oasis.opendocument.spreadsheet")) { return "ods"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.oasis.opendocument.presentation")) { return "odp"; }
    if (mtypEqIgnoreCase(essence, "application/epub+zip")) { return "epub"; }

    // Archives & Binaries
    if (mtypEqIgnoreCase(essence, "application/zip")) { return "zip"; }
    if (mtypEqIgnoreCase(essence, "application/gzip")) { return "gz"; }
    if (mtypEqIgnoreCase(essence, "application/x-tar")) { return "tar"; }
    if (mtypEqIgnoreCase(essence, "application/x-brotli")) { return "br"; }
    if (mtypEqIgnoreCase(essence, "application/zstd")) { return "zstd"; }
    if (mtypEqIgnoreCase(essence, "application/x-7z-compressed")) { return "7z"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.rar")) { return "rar"; }
    if (mtypEqIgnoreCase(essence, "application/x-bzip2")) { return "bz2"; }
    if (mtypEqIgnoreCase(essence, "application/x-xz")) { return "xz"; }
    if (mtypEqIgnoreCase(essence, "application/wasm")) { return "wasm"; }
    if (mtypEqIgnoreCase(essence, "application/octet-stream")) { return "bin"; }
    if (mtypEqIgnoreCase(essence, "application/x-msdownload")) { return "exe"; }
    if (mtypEqIgnoreCase(essence, "application/x-apple-diskimage")) { return "dmg"; }
    if (mtypEqIgnoreCase(essence, "application/x-iso9660-image")) { return "iso"; }
    if (mtypEqIgnoreCase(essence, "application/x-debian-package")) { return "deb"; }
    if (mtypEqIgnoreCase(essence, "application/x-redhat-package-manager")) { return "rpm"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.android.package-archive")) { return "apk"; }
    if (mtypEqIgnoreCase(essence, "application/java-archive")) { return "jar"; }

    // Audio
    if (mtypEqIgnoreCase(essence, "audio/mpeg")) { return "mp3"; }
    if (mtypEqIgnoreCase(essence, "audio/wav")) { return "wav"; }
    if (mtypEqIgnoreCase(essence, "audio/ogg")) { return "ogg"; }
    if (mtypEqIgnoreCase(essence, "audio/opus")) { return "opus"; }
    if (mtypEqIgnoreCase(essence, "audio/flac")) { return "flac"; }
    if (mtypEqIgnoreCase(essence, "audio/aac")) { return "aac"; }
    if (mtypEqIgnoreCase(essence, "audio/mp4")) { return "m4a"; }
    if (mtypEqIgnoreCase(essence, "audio/webm")) { return "weba"; }
    if (mtypEqIgnoreCase(essence, "audio/midi")) { return "midi"; }

    // Video
    if (mtypEqIgnoreCase(essence, "video/mp4")) { return "mp4"; }
    if (mtypEqIgnoreCase(essence, "video/webm")) { return "webm"; }
    if (mtypEqIgnoreCase(essence, "video/quicktime")) { return "mov"; }
    if (mtypEqIgnoreCase(essence, "video/x-msvideo")) { return "avi"; }
    if (mtypEqIgnoreCase(essence, "video/x-matroska")) { return "mkv"; }
    if (mtypEqIgnoreCase(essence, "video/x-ms-wmv")) { return "wmv"; }
    if (mtypEqIgnoreCase(essence, "video/x-flv")) { return "flv"; }
    if (mtypEqIgnoreCase(essence, "video/ogg")) { return "ogv"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.apple.mpegurl")) { return "m3u8"; }

    // Fonts
    if (mtypEqIgnoreCase(essence, "font/woff")) { return "woff"; }
    if (mtypEqIgnoreCase(essence, "font/woff2")) { return "woff2"; }
    if (mtypEqIgnoreCase(essence, "font/ttf")) { return "ttf"; }
    if (mtypEqIgnoreCase(essence, "font/otf")) { return "otf"; }
    if (mtypEqIgnoreCase(essence, "application/vnd.ms-fontobject")) { return "eot"; }

    // Config & Code
    if (mtypEqIgnoreCase(essence, "text/yaml") || mtypEqIgnoreCase(essence, "application/yaml") || mtypEqIgnoreCase(essence, "application/x-yaml")) { return "yaml"; }
    if (mtypEqIgnoreCase(essence, "application/toml")) { return "toml"; }
    if (mtypEqIgnoreCase(essence, "application/x-sh")) { return "sh"; }
    if (mtypEqIgnoreCase(essence, "text/x-python")) { return "py"; }
    if (mtypEqIgnoreCase(essence, "text/x-rust")) { return "rs"; }
    if (mtypEqIgnoreCase(essence, "text/x-cp")) { return "cp"; }
    if (mtypEqIgnoreCase(essence, "text/x-zig")) { return "zig"; }
    if (mtypEqIgnoreCase(essence, "text/x-c")) { return "c"; }
    if (mtypEqIgnoreCase(essence, "text/x-c++")) { return "cpp"; }
    if (mtypEqIgnoreCase(essence, "text/x-go")) { return "go"; }
    if (mtypEqIgnoreCase(essence, "text/x-java-source")) { return "java"; }
    if (mtypEqIgnoreCase(essence, "text/x-ruby")) { return "rb"; }
    if (mtypEqIgnoreCase(essence, "application/x-httpd-php")) { return "php"; }
    if (mtypEqIgnoreCase(essence, "application/sql")) { return "sql"; }
    if (mtypEqIgnoreCase(essence, "application/graphql")) { return "graphql"; }
    if (mtypEqIgnoreCase(essence, "text/x-protobuf")) { return "proto"; }
    if (mtypEqIgnoreCase(essence, "application/x-x509-ca-cert")) { return "pem"; }

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
    let ts: usize = 0;
    let te: usize = 0;
    mtypTrimBounds(raw, &ts, &te);
    if (ts >= te) {
        return empty;
    }
    const s: string = raw[ts..te];

    // Split at first ';' to separate essence from parameters
    let semi: usize = 0;
    while (semi < s.len && s[semi] != 59) {
        semi = semi + 1;
    }
    let es_s: usize = 0;
    let es_e: usize = 0;
    mtypTrimBounds(s[0..semi], &es_s, &es_e);
    if (es_s >= es_e) {
        return empty;
    }
    const essence: string = s[es_s..es_e];

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
        let next_semi: usize = p;
        while (next_semi < s.len && s[next_semi] != 59) {
            next_semi = next_semi + 1;
        }
        const seg: string = s[p..next_semi];
        // Find '='
        let eq: i64 = -1;
        let k: usize = 0;
        while (k < seg.len) {
            if (seg[k] == 61) {
                eq = k as i64;
                break;
            }
            k = k + 1;
        }
        if (eq > 0) {
            const eq_idx: usize = eq as usize;
            let ks: usize = 0;
            let ke: usize = 0;
            mtypTrimBounds(seg[0..eq_idx], &ks, &ke);
            const key: string = seg[ks..ke];
            if (mtypEqIgnoreCase(key, "charset")) {
                let vs: usize = 0;
                let ve: usize = 0;
                mtypTrimBounds(seg[(eq_idx + 1)..seg.len], &vs, &ve);
                const raw_val: string = seg[(eq_idx + 1 + vs)..(eq_idx + 1 + ve)];
                if (raw_val.len >= 2 && ((raw_val[0] == 34 && raw_val[raw_val.len - 1] == 34) ||
                    (raw_val[0] == 39 && raw_val[raw_val.len - 1] == 39))) {
                    charset_val = raw_val[1..(raw_val.len - 1)];
                } else {
                    charset_val = raw_val;
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
    if (mtypEqIgnoreCase(parts.type_part, "text")) {
        return "UTF-8";
    }
    if (mtypEqIgnoreCase(parts.essence, "application/json") ||
        mtypEqIgnoreCase(parts.essence, "application/javascript") ||
        mtypEqIgnoreCase(parts.essence, "application/ecmascript") ||
        mtypEqIgnoreCase(parts.essence, "application/xml") ||
        mtypEqIgnoreCase(parts.essence, "application/xhtml+xml") ||
        mtypEqIgnoreCase(parts.essence, "application/graphql") ||
        mtypEqIgnoreCase(parts.essence, "application/sql") ||
        mtypEqIgnoreCase(parts.essence, "application/toml") ||
        mtypEqIgnoreCase(parts.essence, "application/yaml") ||
        mtypEqIgnoreCase(parts.essence, "application/x-yaml") ||
        mtypEqIgnoreCase(parts.essence, "application/x-www-form-urlencoded") ||
        mtypEqIgnoreCase(parts.essence, "image/svg+xml")) {
        return "UTF-8";
    }
    if (mtypEndsWithIgnoreCase(parts.subtype, "+json") ||
        mtypEndsWithIgnoreCase(parts.subtype, "+xml")) {
        return "UTF-8";
    }
    return null;
}

fn mtypIsTopLevelType(t: string): bool {
    return mtypEqIgnoreCase(t, "text") ||
        mtypEqIgnoreCase(t, "image") ||
        mtypEqIgnoreCase(t, "audio") ||
        mtypEqIgnoreCase(t, "video") ||
        mtypEqIgnoreCase(t, "font") ||
        mtypEqIgnoreCase(t, "application") ||
        mtypEqIgnoreCase(t, "multipart") ||
        mtypEqIgnoreCase(t, "message") ||
        mtypEqIgnoreCase(t, "model") ||
        mtypEqIgnoreCase(t, "example") ||
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
    let ts: usize = 0;
    let te: usize = 0;
    mtypTrimBounds(type_or_ext, &ts, &te);
    if (ts >= te) {
        return 0;
    }
    const trimmed: string = type_or_ext[ts..te];

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

    let pos: usize = 0;
    mtypWriteStr(out, &pos, base_mime);
    if (!has_charset) {
        const cs = charset(base_mime);
        if (cs != null) {
            mtypWriteStr(out, &pos, "; charset=utf-8");
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
    if (mtypEqIgnoreCase(parts.type_part, "text")) {
        return true;
    }
    if (mtypEndsWithIgnoreCase(parts.subtype, "+json") ||
        mtypEndsWithIgnoreCase(parts.subtype, "+xml") ||
        mtypEndsWithIgnoreCase(parts.subtype, "+text")) {
        return true;
    }
    if (mtypEqIgnoreCase(parts.essence, "application/json") ||
        mtypEqIgnoreCase(parts.essence, "application/javascript") ||
        mtypEqIgnoreCase(parts.essence, "application/ecmascript") ||
        mtypEqIgnoreCase(parts.essence, "application/xml") ||
        mtypEqIgnoreCase(parts.essence, "application/wasm") ||
        mtypEqIgnoreCase(parts.essence, "application/graphql") ||
        mtypEqIgnoreCase(parts.essence, "application/sql") ||
        mtypEqIgnoreCase(parts.essence, "application/toml") ||
        mtypEqIgnoreCase(parts.essence, "application/yaml") ||
        mtypEqIgnoreCase(parts.essence, "application/x-yaml") ||
        mtypEqIgnoreCase(parts.essence, "application/rtf") ||
        mtypEqIgnoreCase(parts.essence, "application/x-sh") ||
        mtypEqIgnoreCase(parts.essence, "application/x-www-form-urlencoded") ||
        mtypEqIgnoreCase(parts.essence, "application/vnd.apple.mpegurl") ||
        mtypEqIgnoreCase(parts.essence, "application/vnd.ms-fontobject") ||
        mtypEqIgnoreCase(parts.essence, "image/svg+xml") ||
        mtypEqIgnoreCase(parts.essence, "image/bmp") ||
        mtypEqIgnoreCase(parts.essence, "image/x-icon") ||
        mtypEqIgnoreCase(parts.essence, "image/vnd.microsoft.icon") ||
        mtypEqIgnoreCase(parts.essence, "font/ttf") ||
        mtypEqIgnoreCase(parts.essence, "font/otf")) {
        return true;
    }
    return false;
}
