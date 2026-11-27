// Source: Absorbed from /data/dev/bun/src/http_types/{MimeType,mime_type_list_enum}.cp | Synced: 2026-11-21T17:06:09Z

/// MIME 内容主分类
export enum MimeCategory {
    text,
    javascript,
    json,
    wasm,
    image,
    audio,
    video,
    font,
    archive,
    application,
    multipart,
    other,
}

type MimeLkVec16 = @Vector(16, u8);

fn makeMimeLkLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

fn makeMimeLkTcharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
            || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
            || c == '^' || c == '_' || c == '`' || c == '|' || c == '~') {
            lut[i] = 1;
        }
    }
    return lut;
}

const MIME_LK_LOWER_LUT: [u8; 256] = makeMimeLkLowerLut();
const MIME_LK_TCHAR_LUT: [u8; 256] = makeMimeLkTcharLut();

fn toLowerAscii(c: u8): u8 {
    return MIME_LK_LOWER_LUT[usize(c)];
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: MimeLkVec16 = @splat(65 as u8);
    const hi_Z: MimeLkVec16 = @splat(90 as u8);
    const case_bit: MimeLkVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: MimeLkVec16 = a[i .. i + 16][0..16].*;
        const vb: MimeLkVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) : (i += 1) {
        if (MIME_LK_LOWER_LUT[usize(a[i])] != MIME_LK_LOWER_LUT[usize(b[i])]) {
            return false;
        }
    }
    return true;
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) {
        return false;
    }
    return asciiEqlIgnoreCase(s[s.len - suffix.len..], suffix);
}

fn isTokenChar(c: u8): bool {
    return MIME_LK_TCHAR_LUT[usize(c)] != 0;
}

fn isValidToken(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (MIME_LK_TCHAR_LUT[usize(s[i])] == 0) {
            return false;
        }
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    const v_n: MimeLkVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: MimeLkVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_n);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) {
            return i;
        }
    }
    return null;
}

fn mimeLkSimdFindAny2(s: []const u8, start: usize, a: u8, b: u8): usize {
    let i = start;
    const va: MimeLkVec16 = @splat(a);
    const vb: MimeLkVec16 = @splat(b);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: MimeLkVec16 = s[i .. i + 16][0..16].*;
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mask: u16 = ma | mb;
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == a || s[i] == b) return i;
    }
    return s.len;
}

/// 解析后的 HTTP Content-Type 头部（零拷贝切片）
export struct ContentType {
    media_type: []const u8;
    type_part: []const u8;
    subtype: []const u8;
    charset: ([]const u8)?;
    boundary: ([]const u8)?;

    /// 解析形如 `type/subtype; charset=utf-8; boundary="---abc"` 的 Content-Type 头部
    static fn parse(header: []const u8): ContentType? {
        const trimmed = trimAscii(header);
        if (trimmed.len == 0) {
            return null;
        }

        // 查找首个分号切分 media_type 与参数列表
        let semi_pos: usize = trimmed.len;
        if (idx := findByte(trimmed, ';')) {
            semi_pos = idx;
        }
        const media_type = trimAscii(trimmed[0..semi_pos]);
        const slash_idx = findByte(media_type, '/') ?? return null;
        const type_part = media_type[0..slash_idx];
        const subtype = media_type[slash_idx + 1..];
        if (!isValidToken(type_part) || !isValidToken(subtype)) {
            return null;
        }

        let charset: ([]const u8)? = null;
        let boundary: ([]const u8)? = null;

        // 解析参数（尊重双引号内的分号）
        let i: usize = semi_pos;
        if (i < trimmed.len && trimmed[i] == ';') {
            i += 1;
        }
        while (i < trimmed.len) {
            const seg_start = i;
            let in_quote = false;
            let escaped = false;
            while (i < trimmed.len) {
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                        i += 1;
                    } else {
                        i = mimeLkSimdFindAny2(trimmed, i, '\\', '"');
                        if (i >= trimmed.len) break;
                        if (trimmed[i] == '\\') {
                            escaped = true;
                        } else {
                            in_quote = false;
                        }
                        i += 1;
                    }
                } else {
                    i = mimeLkSimdFindAny2(trimmed, i, '"', ';');
                    if (i >= trimmed.len) break;
                    if (trimmed[i] == '"') {
                        in_quote = true;
                        i += 1;
                    } else {
                        break;
                    }
                }
            }
            const param = trimAscii(trimmed[seg_start..i]);
            if (i < trimmed.len && trimmed[i] == ';') {
                i += 1;
            }
            if (param.len == 0) {
                continue;
            }

            if (eq_idx := findByte(param, '=')) {
                const key = trimAscii(param[0..eq_idx]);
                let val = trimAscii(param[eq_idx + 1..]);
                if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                    val = val[1..val.len - 1];
                }
                if (asciiEqlIgnoreCase(key, "charset")) {
                    charset = val;
                } else if (asciiEqlIgnoreCase(key, "boundary")) {
                    boundary = val;
                }
            }
        }

        return ContentType{
            media_type: media_type,
            type_part: type_part,
            subtype: subtype,
            charset: charset,
            boundary: boundary,
        };
    }

    /// 是否为 `text/*` 主类型
    fn isText(): bool {
        return asciiEqlIgnoreCase(_.type_part, "text");
    }

    /// 是否为 JSON 或 `+json` 结构化后缀类型
    fn isJson(): bool {
        if (asciiEqlIgnoreCase(_.subtype, "json") && (asciiEqlIgnoreCase(_.type_part, "application") || asciiEqlIgnoreCase(_.type_part, "text"))) {
            return true;
        }
        return endsWithIgnoreCase(_.subtype, "+json");
    }

    /// 是否为 `multipart/*` 类型
    fn isMultipart(): bool {
        return asciiEqlIgnoreCase(_.type_part, "multipart");
    }

    /// 是否适合 HTTP 响应压缩（gzip / br / zstd）
    fn isCompressible(): bool {
        if (_.isText() || _.isJson()) {
            return true;
        }
        if (endsWithIgnoreCase(_.subtype, "+xml")) {
            return true;
        }
        if (asciiEqlIgnoreCase(_.type_part, "application")) {
            if (asciiEqlIgnoreCase(_.subtype, "javascript")
                || asciiEqlIgnoreCase(_.subtype, "x-javascript")
                || asciiEqlIgnoreCase(_.subtype, "ecmascript")
                || asciiEqlIgnoreCase(_.subtype, "xml")
                || asciiEqlIgnoreCase(_.subtype, "wasm")
                || asciiEqlIgnoreCase(_.subtype, "toml")
                || asciiEqlIgnoreCase(_.subtype, "yaml")
                || asciiEqlIgnoreCase(_.subtype, "x-yaml")
                || asciiEqlIgnoreCase(_.subtype, "x-www-form-urlencoded")
                || asciiEqlIgnoreCase(_.subtype, "vnd.ms-fontobject"))
            {
                return true;
            }
        }
        if (asciiEqlIgnoreCase(_.type_part, "font")) {
            if (asciiEqlIgnoreCase(_.subtype, "ttf") || asciiEqlIgnoreCase(_.subtype, "otf")) {
                return true;
            }
        }
        return false;
    }
}

fn extractExtension(ext_or_path: []const u8): []const u8 {
    let s = ext_or_path;
    if (q := findByte(s, '?')) {
        s = s[0..q];
    }
    if (h := findByte(s, '#')) {
        s = s[0..h];
    }
    if (s.len == 0) {
        return "";
    }

    // 找到末段文件名起点（最后一个 '/' 或 '\\'）
    let base_start: usize = 0;
    let has_sep = false;
    let i: usize = 0;
    const v_sl: MimeLkVec16 = @splat('/' as u8);
    const v_bs: MimeLkVec16 = @splat('\\' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: MimeLkVec16 = s[i .. i + 16][0..16].*;
        const m1: u16 = @bitCast(chunk == v_sl);
        const m2: u16 = @bitCast(chunk == v_bs);
        const mask: u16 = m1 | m2;
        if (mask != 0) {
            base_start = i + 16 - usize(@clz(mask));
            has_sep = true;
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == '/' || s[i] == '\\') {
            base_start = i + 1;
            has_sep = true;
        }
    }
    const base = s[base_start..];
    if (base.len == 0) {
        return "";
    }

    // 在末段文件名中查找最后一个 '.'
    let dot_pos: usize? = null;
    let j: usize = 0;
    const v_dot: MimeLkVec16 = @splat('.' as u8);
    while (j + 16 <= base.len) : (j += 16) {
        const chunk: MimeLkVec16 = base[j .. j + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_dot);
        if (mask != 0) {
            dot_pos = j + 15 - usize(@clz(mask));
        }
    }
    while (j < base.len) : (j += 1) {
        if (base[j] == '.') {
            dot_pos = j;
        }
    }

    if (dp := dot_pos) {
        if (dp + 1 >= base.len) {
            return "";
        }
        return base[dp + 1..];
    }

    // 不含路径分隔符且不含 '.' 时，按裸扩展名处理（如 "json" / "WASM"）
    if (!has_sep) {
        return base;
    }
    return "";
}

/// 根据扩展名或完整文件路径查找标准 MIME 类型（大小写无关，覆盖 65+ 高频扩展名）
export fn mimeFromExtension(ext_or_path: []const u8): []const u8 {
    const ext = extractExtension(ext_or_path);
    if (ext.len == 0) {
        return "application/octet-stream";
    }

    // Web & 文本
    if (asciiEqlIgnoreCase(ext, "html") || asciiEqlIgnoreCase(ext, "htm")) return "text/html; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "css")) return "text/css; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "js") || asciiEqlIgnoreCase(ext, "mjs") || asciiEqlIgnoreCase(ext, "cjs")) return "text/javascript; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "ts") || asciiEqlIgnoreCase(ext, "mts") || asciiEqlIgnoreCase(ext, "cts")) return "text/typescript; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "tsx")) return "text/tsx; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "jsx")) return "text/jsx; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "json") || asciiEqlIgnoreCase(ext, "jsonc") || asciiEqlIgnoreCase(ext, "map")) return "application/json; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "webmanifest")) return "application/manifest+json; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "xml")) return "application/xml; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "svg")) return "image/svg+xml";
    if (asciiEqlIgnoreCase(ext, "txt")) return "text/plain; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "md") || asciiEqlIgnoreCase(ext, "markdown")) return "text/markdown; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "csv")) return "text/csv; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "tsv")) return "text/tab-separated-values; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "yaml") || asciiEqlIgnoreCase(ext, "yml")) return "application/yaml; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "toml")) return "application/toml; charset=utf-8";
    if (asciiEqlIgnoreCase(ext, "wasm")) return "application/wasm";

    // 图片
    if (asciiEqlIgnoreCase(ext, "png")) return "image/png";
    if (asciiEqlIgnoreCase(ext, "jpg") || asciiEqlIgnoreCase(ext, "jpeg")) return "image/jpeg";
    if (asciiEqlIgnoreCase(ext, "gif")) return "image/gif";
    if (asciiEqlIgnoreCase(ext, "webp")) return "image/webp";
    if (asciiEqlIgnoreCase(ext, "avif")) return "image/avif";
    if (asciiEqlIgnoreCase(ext, "ico")) return "image/x-icon";
    if (asciiEqlIgnoreCase(ext, "bmp")) return "image/bmp";
    if (asciiEqlIgnoreCase(ext, "tiff") || asciiEqlIgnoreCase(ext, "tif")) return "image/tiff";

    // 音频
    if (asciiEqlIgnoreCase(ext, "mp3")) return "audio/mpeg";
    if (asciiEqlIgnoreCase(ext, "wav")) return "audio/wav";
    if (asciiEqlIgnoreCase(ext, "ogg") || asciiEqlIgnoreCase(ext, "oga")) return "audio/ogg";
    if (asciiEqlIgnoreCase(ext, "flac")) return "audio/flac";
    if (asciiEqlIgnoreCase(ext, "m4a")) return "audio/mp4";
    if (asciiEqlIgnoreCase(ext, "aac")) return "audio/aac";
    if (asciiEqlIgnoreCase(ext, "opus")) return "audio/opus";

    // 视频
    if (asciiEqlIgnoreCase(ext, "mp4") || asciiEqlIgnoreCase(ext, "m4v")) return "video/mp4";
    if (asciiEqlIgnoreCase(ext, "webm")) return "video/webm";
    if (asciiEqlIgnoreCase(ext, "mov")) return "video/quicktime";
    if (asciiEqlIgnoreCase(ext, "avi")) return "video/x-msvideo";
    if (asciiEqlIgnoreCase(ext, "mkv")) return "video/x-matroska";

    // 字体
    if (asciiEqlIgnoreCase(ext, "woff")) return "font/woff";
    if (asciiEqlIgnoreCase(ext, "woff2")) return "font/woff2";
    if (asciiEqlIgnoreCase(ext, "ttf")) return "font/ttf";
    if (asciiEqlIgnoreCase(ext, "otf")) return "font/otf";
    if (asciiEqlIgnoreCase(ext, "eot")) return "application/vnd.ms-fontobject";

    // 文档与压缩归档
    if (asciiEqlIgnoreCase(ext, "pdf")) return "application/pdf";
    if (asciiEqlIgnoreCase(ext, "zip")) return "application/zip";
    if (asciiEqlIgnoreCase(ext, "gz") || asciiEqlIgnoreCase(ext, "gzip")) return "application/gzip";
    if (asciiEqlIgnoreCase(ext, "tar")) return "application/x-tar";
    if (asciiEqlIgnoreCase(ext, "br")) return "application/brotli";
    if (asciiEqlIgnoreCase(ext, "zst")) return "application/zstd";
    if (asciiEqlIgnoreCase(ext, "7z")) return "application/x-7z-compressed";
    if (asciiEqlIgnoreCase(ext, "bz2")) return "application/x-bzip2";
    if (asciiEqlIgnoreCase(ext, "xz")) return "application/x-xz";

    // 源码与脚本
    if (asciiEqlIgnoreCase(ext, "cp")
        || asciiEqlIgnoreCase(ext, "zig")
        || asciiEqlIgnoreCase(ext, "rs")
        || asciiEqlIgnoreCase(ext, "c")
        || asciiEqlIgnoreCase(ext, "h")
        || asciiEqlIgnoreCase(ext, "cpp")
        || asciiEqlIgnoreCase(ext, "py")
        || asciiEqlIgnoreCase(ext, "ini")
        || asciiEqlIgnoreCase(ext, "sql"))
    {
        return "text/plain; charset=utf-8";
    }
    if (asciiEqlIgnoreCase(ext, "sh")) return "application/x-sh";

    return "application/octet-stream";
}

/// 根据 MIME 类型字符串归类到 MimeCategory
export fn categoryFromMime(mime: []const u8): MimeCategory {
    const ct = ContentType.parse(mime) ?? return MimeCategory.other;

    if (ct.isJson()) {
        return MimeCategory.json;
    }
    if (asciiEqlIgnoreCase(ct.subtype, "javascript")
        || asciiEqlIgnoreCase(ct.subtype, "x-javascript")
        || asciiEqlIgnoreCase(ct.subtype, "ecmascript"))
    {
        return MimeCategory.javascript;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "application") && asciiEqlIgnoreCase(ct.subtype, "wasm")) {
        return MimeCategory.wasm;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "text")) {
        return MimeCategory.text;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "image")) {
        return MimeCategory.image;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "audio")) {
        return MimeCategory.audio;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "video")) {
        return MimeCategory.video;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "font")) {
        return MimeCategory.font;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "multipart")) {
        return MimeCategory.multipart;
    }
    if (asciiEqlIgnoreCase(ct.type_part, "application")) {
        if (asciiEqlIgnoreCase(ct.subtype, "zip")
            || asciiEqlIgnoreCase(ct.subtype, "gzip")
            || asciiEqlIgnoreCase(ct.subtype, "x-tar")
            || asciiEqlIgnoreCase(ct.subtype, "brotli")
            || asciiEqlIgnoreCase(ct.subtype, "zstd")
            || asciiEqlIgnoreCase(ct.subtype, "x-7z-compressed")
            || asciiEqlIgnoreCase(ct.subtype, "x-bzip2")
            || asciiEqlIgnoreCase(ct.subtype, "x-xz"))
        {
            return MimeCategory.archive;
        }
        return MimeCategory.application;
    }
    return MimeCategory.other;
}
