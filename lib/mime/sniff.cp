// Source: Absorbed from /data/dev/bun/src/http/MimeType.zig | Synced: 2026-11-22T11:12:13Z (WHATWG MIME Sniffing)

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn startsWith(buf: []const u8, prefix: []const u8): bool {
    if (buf.len < prefix.len) { return false; }
    return eqlBytes(buf[0..prefix.len], prefix);
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCaseAscii(buf: []const u8, prefix: []const u8): bool {
    if (buf.len < prefix.len) { return false; }
    return eqlIgnoreCaseAscii(buf[0..prefix.len], prefix);
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn isTagTerm(c: u8): bool {
    return isSpace(c) || c == '>' || c == '/';
}

fn stripBomAndLeadSpace(buf: []const u8): []const u8 {
    let i: usize = 0;
    if (buf.len >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
        i = 3;
    }
    while (i < buf.len && isSpace(buf[i])) {
        i += 1;
    }
    return buf[i..buf.len];
}

fn trimEndSpace(buf: []const u8): []const u8 {
    let end = buf.len;
    while (end > 0 && isSpace(buf[end - 1])) {
        end -= 1;
    }
    return buf[0..end];
}

/// 遵循 WHATWG MIME Sniffing §3 判断字节是否为二进制数据字节
pub fn isBinaryDataByte(b: u8): bool {
    if (b <= 0x08) { return true; }
    if (b == 0x0B) { return true; }
    if (b >= 0x0E && b <= 0x1A) { return true; }
    if (b >= 0x1C && b <= 0x1F) { return true; }
    if (b == 0x7F) { return true; }
    return false;
}

/// 检查前 512 字节是否为合法文本数据（支持 UTF-8/UTF-16 BOM，拒绝二进制控制字节与非法 UTF-8 序列）
pub fn isTextData(buf: []const u8): bool {
    if (buf.len == 0) {
        return true;
    }
    // UTF-16BE / UTF-16LE BOM 视为文本
    if (buf.len >= 2 && ((buf[0] == 0xFE && buf[1] == 0xFF) || (buf[0] == 0xFF && buf[1] == 0xFE))) {
        return true;
    }

    let start: usize = 0;
    if (buf.len >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
        start = 3;
    }

    const limit = if (buf.len < 512) buf.len else 512;
    const truncated_window = (buf.len > 512);
    let i = start;

    while (i < limit) {
        const b0 = buf[i];
        if (b0 < 0x80) {
            if (isBinaryDataByte(b0)) {
                return false;
            }
            i += 1;
            continue;
        }

        // UTF-8 多字节校验
        if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 >= limit) { return truncated_window; }
            if ((buf[i + 1] & 0xC0) != 0x80) { return false; }
            i += 2;
        } else if (b0 >= 0xE0 && b0 <= 0xEF) {
            if (i + 2 >= limit) { return truncated_window; }
            const b1 = buf[i + 1];
            const b2 = buf[i + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) { return false; }
            if (b0 == 0xE0 && b1 < 0xA0) { return false; }
            if (b0 == 0xED && b1 >= 0xA0) { return false; } // 拒绝 UTF-16 代理项
            i += 3;
        } else if (b0 >= 0xF0 && b0 <= 0xF4) {
            if (i + 3 >= limit) { return truncated_window; }
            const b1 = buf[i + 1];
            const b2 = buf[i + 2];
            const b3 = buf[i + 3];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) { return false; }
            if (b0 == 0xF0 && b1 < 0x90) { return false; }
            if (b0 == 0xF4 && b1 >= 0x90) { return false; }
            i += 4;
        } else {
            return false;
        }
    }

    return true;
}

/// 嗅探常见图像格式魔数（PNG/JPEG/GIF87a/GIF89a/WebP/BMP/ICO/SVG）
pub fn sniffImageMime(buf: []const u8): ([]const u8)? {
    if (startsWith(buf, "\x89PNG\r\n\x1A\n")) {
        return "image/png";
    }
    if (buf.len >= 3 && buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF) {
        return "image/jpeg";
    }
    if (startsWith(buf, "GIF87a") || startsWith(buf, "GIF89a")) {
        return "image/gif";
    }
    if (buf.len >= 12 && startsWith(buf, "RIFF") && eqlBytes(buf[8..12], "WEBP")) {
        return "image/webp";
    }
    if (startsWith(buf, "BM")) {
        return "image/bmp";
    }
    if (buf.len >= 4 && buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01 && buf[3] == 0x00) {
        return "image/x-icon";
    }

    // SVG 文本魔数（支持可选 XML 声明与注释前缀）
    let s = stripBomAndLeadSpace(buf);
    if (startsWithIgnoreCaseAscii(s, "<?xml")) {
        let i: usize = 5;
        while (i + 1 < s.len) {
            if (s[i] == '?' && s[i + 1] == '>') {
                s = stripBomAndLeadSpace(s[(i + 2)..s.len]);
                break;
            }
            i += 1;
        }
    }
    if (startsWithIgnoreCaseAscii(s, "<svg") && s.len > 4 && isTagTerm(s[4])) {
        return "image/svg+xml";
    }

    return null;
}

/// 嗅探归档、文档与可执行文件魔数（PDF/ZIP/GZIP/ZSTD/BZIP2/XZ/WASM/ELF/SQLite）
pub fn sniffArchiveOrExecMime(buf: []const u8): ([]const u8)? {
    if (startsWith(buf, "%PDF-")) {
        return "application/pdf";
    }
    if (startsWith(buf, "PK\x03\x04") || startsWith(buf, "PK\x05\x06")) {
        return "application/zip";
    }
    if (buf.len >= 3 && buf[0] == 0x1F && buf[1] == 0x8B && buf[2] == 0x08) {
        return "application/gzip";
    }
    if (buf.len >= 4 && buf[0] == 0x28 && buf[1] == 0xB5 && buf[2] == 0x2F && buf[3] == 0xFD) {
        return "application/zstd";
    }
    if (startsWith(buf, "BZh")) {
        return "application/x-bzip2";
    }
    if (startsWith(buf, "\xFD7zXZ\x00")) {
        return "application/x-xz";
    }
    if (startsWith(buf, "\x00asm")) {
        return "application/wasm";
    }
    if (startsWith(buf, "\x7FELF")) {
        return "application/x-elf";
    }
    if (startsWith(buf, "SQLite format 3\x00")) {
        return "application/vnd.sqlite3";
    }
    return null;
}

/// 嗅探音视频格式魔数（WAV/OGG/FLAC/MP3 ID3/MP4 ftyp/WebM EBML）
pub fn sniffAudioVideoMime(buf: []const u8): ([]const u8)? {
    if (buf.len >= 12 && startsWith(buf, "RIFF") && eqlBytes(buf[8..12], "WAVE")) {
        return "audio/wav";
    }
    if (startsWith(buf, "OggS\x00")) {
        return "application/ogg";
    }
    if (startsWith(buf, "fLaC")) {
        return "audio/flac";
    }
    if (startsWith(buf, "ID3")) {
        return "audio/mpeg";
    }
    if (buf.len >= 2 && buf[0] == 0xFF && (buf[1] == 0xFB || buf[1] == 0xF3 || buf[1] == 0xF2)) {
        return "audio/mpeg";
    }
    if (buf.len >= 12 && eqlBytes(buf[4..8], "ftyp")) {
        return "video/mp4";
    }
    if (buf.len >= 4 && buf[0] == 0x1A && buf[1] == 0x45 && buf[2] == 0xDF && buf[3] == 0xA3) {
        return "video/webm";
    }
    return null;
}

fn looksLikeHtml(s: []const u8): bool {
    if (startsWithIgnoreCaseAscii(s, "<!DOCTYPE html") && (s.len == 14 || isTagTerm(s[14]))) {
        return true;
    }
    if (startsWithIgnoreCaseAscii(s, "<html") && s.len > 5 && isTagTerm(s[5])) { return true; }
    if (startsWithIgnoreCaseAscii(s, "<head") && s.len > 5 && isTagTerm(s[5])) { return true; }
    if (startsWithIgnoreCaseAscii(s, "<body") && s.len > 5 && isTagTerm(s[5])) { return true; }
    if (startsWithIgnoreCaseAscii(s, "<script") && s.len > 7 && isTagTerm(s[7])) { return true; }
    if (startsWithIgnoreCaseAscii(s, "<title") && s.len > 6 && isTagTerm(s[6])) { return true; }
    if (startsWithIgnoreCaseAscii(s, "<div") && s.len > 4 && isTagTerm(s[4])) { return true; }
    return false;
}

fn looksLikeJson(s: []const u8): bool {
    const t = trimEndSpace(s);
    if (t.len < 2) { return false; }
    if (t[0] == '{' && t[t.len - 1] == '}') {
        let i: usize = 1;
        while (i < t.len && isSpace(t[i])) { i += 1; }
        return i < t.len && (t[i] == '"' || t[i] == '}');
    }
    if (t[0] == '[' && t[t.len - 1] == ']') {
        let i: usize = 1;
        while (i < t.len && isSpace(t[i])) { i += 1; }
        if (i >= t.len) { return false; }
        const c = t[i];
        return c == ']' || c == '"' || c == '{' || c == '[' || c == '-' ||
            (c >= '0' && c <= '9') || c == 't' || c == 'f' || c == 'n';
    }
    return false;
}

/// 综合 MIME 嗅探：图像 -> 音视频 -> 归档/可执行 -> HTML/XML/JSON 文本探测 -> `text/plain; charset=utf-8` 或 `application/octet-stream`
pub fn sniffMime(buf: []const u8): []const u8 {
    if (m := sniffImageMime(buf)) { return m; }
    if (m := sniffAudioVideoMime(buf)) { return m; }
    if (m := sniffArchiveOrExecMime(buf)) { return m; }

    if (isTextData(buf)) {
        const s = stripBomAndLeadSpace(buf);
        if (looksLikeHtml(s)) {
            return "text/html; charset=utf-8";
        }
        if (startsWithIgnoreCaseAscii(s, "<?xml")) {
            return "application/xml";
        }
        if (looksLikeJson(s)) {
            return "application/json";
        }
        return "text/plain; charset=utf-8";
    }

    return "application/octet-stream";
}
