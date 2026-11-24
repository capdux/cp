// Origin: Pure-CP Stdlib (cap:web Zero-Heap Magic-Number File Type Detector) | Created: 2026-11-25
// ============================================================================
// Pure-CP zero-heap binary magic-number & text heuristic file-type detector.
// Covers 30 common Web, image, document, archive, audio/video, font & DB formats
// (1:1 replacement for Elysia's `file-type` dependency).
// ============================================================================

pub enum FileKind {
    Png,
    Jpeg,
    Gif,
    Webp,
    Avif,
    Bmp,
    Ico,
    Pdf,
    Zip,
    Gzip,
    Zstd,
    Xz,
    Bz2,
    Lz4,
    Mp4,
    Webm,
    Mp3,
    Flac,
    Ogg,
    Wav,
    Wasm,
    Woff2,
    Woff,
    Ttf,
    Otf,
    Sqlite3,
    Svg,
    Html,
    Xml,
    Json,
}

pub struct FileTypeInfo {
    kind: FileKind = FileKind.Json,
    ext: string = "",
    mime: string = "",
}

fn hasPrefixAt(bytes: string, offset: usize, magic: string): bool {
    if (bytes.len < offset + magic.len) return false;
    return std.mem.eql(u8, bytes[offset .. offset + magic.len], magic);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn hasPrefixAsciiCaseInsensitive(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (toLowerAscii(s[i]) != toLowerAscii(prefix[i])) return false;
    }
    return true;
}

fn containsAsciiCaseInsensitive(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (hasPrefixAsciiCaseInsensitive(haystack[i .. i + needle.len], needle)) {
            return true;
        }
    }
    return false;
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn hasNoControlBinaryPrefix(bytes: string): bool {
    const check_len: usize = if (bytes.len < 64) bytes.len else 64;
    let i: usize = 0;
    while (i < check_len) : (i += 1) {
        const c = bytes[i];
        if (c == 0) return false;
        if (c < 0x09 || (c > 0x0D && c < 0x20)) return false;
    }
    return true;
}

/// Zero-heap magic-number + text heuristic file type detector.
/// Recognizes 30 Web/media/document/archive/font/DB formats safely without panicking on truncated inputs.
pub fn detectFileType(bytes: string): FileTypeInfo? {
    const n: usize = bytes.len;
    if (n < 2) return null;

    // 1. Images
    if (n >= 8 && bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G' && bytes[4] == 0x0D && bytes[5] == 0x0A && bytes[6] == 0x1A && bytes[7] == 0x0A) {
        return FileTypeInfo{ kind: FileKind.Png, ext: "png", mime: "image/png" };
    }
    if (n >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
        return FileTypeInfo{ kind: FileKind.Jpeg, ext: "jpg", mime: "image/jpeg" };
    }
    if (hasPrefixAt(bytes, 0, "GIF87a") || hasPrefixAt(bytes, 0, "GIF89a")) {
        return FileTypeInfo{ kind: FileKind.Gif, ext: "gif", mime: "image/gif" };
    }
    if (n >= 12 && hasPrefixAt(bytes, 0, "RIFF")) {
        if (hasPrefixAt(bytes, 8, "WEBP")) {
            return FileTypeInfo{ kind: FileKind.Webp, ext: "webp", mime: "image/webp" };
        }
        if (hasPrefixAt(bytes, 8, "WAVE")) {
            return FileTypeInfo{ kind: FileKind.Wav, ext: "wav", mime: "audio/wav" };
        }
    }
    if (n >= 12 && hasPrefixAt(bytes, 4, "ftyp")) {
        if (hasPrefixAt(bytes, 8, "avif") || hasPrefixAt(bytes, 8, "avis")) {
            return FileTypeInfo{ kind: FileKind.Avif, ext: "avif", mime: "image/avif" };
        }
        if (hasPrefixAt(bytes, 8, "isom") || hasPrefixAt(bytes, 8, "iso2") || hasPrefixAt(bytes, 8, "mp41") || hasPrefixAt(bytes, 8, "mp42") || hasPrefixAt(bytes, 8, "M4V ") || hasPrefixAt(bytes, 8, "M4A ") || hasPrefixAt(bytes, 8, "avc1") || hasPrefixAt(bytes, 8, "dash") || hasPrefixAt(bytes, 8, "qt  ") || hasPrefixAt(bytes, 8, "MSNV")) {
            return FileTypeInfo{ kind: FileKind.Mp4, ext: "mp4", mime: "video/mp4" };
        }
    }
    if (n >= 14 && bytes[0] == 'B' && bytes[1] == 'M' && bytes[6] == 0 && bytes[7] == 0 && bytes[8] == 0 && bytes[9] == 0) {
        return FileTypeInfo{ kind: FileKind.Bmp, ext: "bmp", mime: "image/bmp" };
    }
    if (n >= 4 && bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x01 && bytes[3] == 0x00) {
        return FileTypeInfo{ kind: FileKind.Ico, ext: "ico", mime: "image/x-icon" };
    }

    // 2. Documents & Archives
    if (hasPrefixAt(bytes, 0, "%PDF-")) {
        return FileTypeInfo{ kind: FileKind.Pdf, ext: "pdf", mime: "application/pdf" };
    }
    if (n >= 4 && bytes[0] == 'P' && bytes[1] == 'K' && ((bytes[2] == 0x03 && bytes[3] == 0x04) || (bytes[2] == 0x05 && bytes[3] == 0x06) || (bytes[2] == 0x07 && bytes[3] == 0x08))) {
        return FileTypeInfo{ kind: FileKind.Zip, ext: "zip", mime: "application/zip" };
    }
    if (n >= 3 && bytes[0] == 0x1F && bytes[1] == 0x8B && bytes[2] == 0x08) {
        return FileTypeInfo{ kind: FileKind.Gzip, ext: "gz", mime: "application/gzip" };
    }
    if (n >= 4 && bytes[0] == 0x28 && bytes[1] == 0xB5 && bytes[2] == 0x2F && bytes[3] == 0xFD) {
        return FileTypeInfo{ kind: FileKind.Zstd, ext: "zst", mime: "application/zstd" };
    }
    if (n >= 6 && bytes[0] == 0xFD && bytes[1] == '7' && bytes[2] == 'z' && bytes[3] == 'X' && bytes[4] == 'Z' && bytes[5] == 0x00) {
        return FileTypeInfo{ kind: FileKind.Xz, ext: "xz", mime: "application/x-xz" };
    }
    if (n >= 4 && bytes[0] == 'B' && bytes[1] == 'Z' && bytes[2] == 'h' && bytes[3] >= '1' && bytes[3] <= '9') {
        return FileTypeInfo{ kind: FileKind.Bz2, ext: "bz2", mime: "application/x-bzip2" };
    }
    if (n >= 4 && bytes[0] == 0x04 && bytes[1] == 0x22 && bytes[2] == 0x4D && bytes[3] == 0x18) {
        return FileTypeInfo{ kind: FileKind.Lz4, ext: "lz4", mime: "application/x-lz4" };
    }

    // 3. Audio / Video
    if (n >= 4 && bytes[0] == 0x1A && bytes[1] == 0x45 && bytes[2] == 0xDF && bytes[3] == 0xA3) {
        return FileTypeInfo{ kind: FileKind.Webm, ext: "webm", mime: "video/webm" };
    }
    if (hasPrefixAt(bytes, 0, "ID3") || (bytes[0] == 0xFF && (bytes[1] == 0xFB || bytes[1] == 0xF3 || bytes[1] == 0xF2))) {
        return FileTypeInfo{ kind: FileKind.Mp3, ext: "mp3", mime: "audio/mpeg" };
    }
    if (hasPrefixAt(bytes, 0, "fLaC")) {
        return FileTypeInfo{ kind: FileKind.Flac, ext: "flac", mime: "audio/flac" };
    }
    if (hasPrefixAt(bytes, 0, "OggS")) {
        return FileTypeInfo{ kind: FileKind.Ogg, ext: "ogg", mime: "audio/ogg" };
    }

    // 4. WebAssembly, Fonts & SQLite3
    if (n >= 4 && bytes[0] == 0x00 && bytes[1] == 'a' && bytes[2] == 's' && bytes[3] == 'm') {
        return FileTypeInfo{ kind: FileKind.Wasm, ext: "wasm", mime: "application/wasm" };
    }
    if (hasPrefixAt(bytes, 0, "wOF2")) {
        return FileTypeInfo{ kind: FileKind.Woff2, ext: "woff2", mime: "font/woff2" };
    }
    if (hasPrefixAt(bytes, 0, "wOFF")) {
        return FileTypeInfo{ kind: FileKind.Woff, ext: "woff", mime: "font/woff" };
    }
    if (n >= 5 && bytes[0] == 0x00 && bytes[1] == 0x01 && bytes[2] == 0x00 && bytes[3] == 0x00 && bytes[4] == 0x00) {
        return FileTypeInfo{ kind: FileKind.Ttf, ext: "ttf", mime: "font/ttf" };
    }
    if (hasPrefixAt(bytes, 0, "OTTO")) {
        return FileTypeInfo{ kind: FileKind.Otf, ext: "otf", mime: "font/otf" };
    }
    if (n >= 16 && hasPrefixAt(bytes, 0, "SQLite format 3") && bytes[15] == 0x00) {
        return FileTypeInfo{ kind: FileKind.Sqlite3, ext: "sqlite", mime: "application/vnd.sqlite3" };
    }

    // 5. Text-based Web Format Heuristics (SVG / HTML / XML / JSON)
    let start: usize = 0;
    if (n >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        start = 3;
    }
    while (start < n && isAsciiSpace(bytes[start])) : (start += 1) {}
    if (start >= n) return null;

    const trimmed = bytes[start..];
    if (!hasNoControlBinaryPrefix(trimmed)) return null;

    if (hasPrefixAsciiCaseInsensitive(trimmed, "<svg")) {
        return FileTypeInfo{ kind: FileKind.Svg, ext: "svg", mime: "image/svg+xml" };
    }
    if (hasPrefixAsciiCaseInsensitive(trimmed, "<!doctype html") || hasPrefixAsciiCaseInsensitive(trimmed, "<html")) {
        return FileTypeInfo{ kind: FileKind.Html, ext: "html", mime: "text/html" };
    }
    if (hasPrefixAsciiCaseInsensitive(trimmed, "<?xml")) {
        const probe_len: usize = if (trimmed.len < 256) trimmed.len else 256;
        if (containsAsciiCaseInsensitive(trimmed[0..probe_len], "<svg")) {
            return FileTypeInfo{ kind: FileKind.Svg, ext: "svg", mime: "image/svg+xml" };
        }
        return FileTypeInfo{ kind: FileKind.Xml, ext: "xml", mime: "application/xml" };
    }
    if (trimmed[0] == '{' || trimmed[0] == '[') {
        let end: usize = trimmed.len;
        while (end > 0 && isAsciiSpace(trimmed[end - 1])) : (end -= 1) {}
        if (end >= 2) {
            const open_ch = trimmed[0];
            const close_ch = trimmed[end - 1];
            if ((open_ch == '{' && close_ch == '}') || (open_ch == '[' && close_ch == ']')) {
                let inner_pos: usize = 1;
                while (inner_pos < end - 1 && isAsciiSpace(trimmed[inner_pos])) : (inner_pos += 1) {}
                if (inner_pos == end - 1) {
                    return FileTypeInfo{ kind: FileKind.Json, ext: "json", mime: "application/json" };
                }
                const first_inner = trimmed[inner_pos];
                if (open_ch == '{' && first_inner == '"') {
                    return FileTypeInfo{ kind: FileKind.Json, ext: "json", mime: "application/json" };
                }
                if (open_ch == '[' && (first_inner == '"' || first_inner == '{' || first_inner == '[' || first_inner == '-' || (first_inner >= '0' && first_inner <= '9') || first_inner == 't' || first_inner == 'f' || first_inner == 'n')) {
                    return FileTypeInfo{ kind: FileKind.Json, ext: "json", mime: "application/json" };
                }
            }
        }
    }

    return null;
}
