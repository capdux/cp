// Origin: Pure-CP Stdlib (Zero-Alloc Gemini Protocol v0.16.1 & Gemtext Codec) | Created: 2026-11-22

pub const GEMINI_TCP_PORT: u16 = 1965;
pub const GEMINI_MAX_URL_LEN: usize = 1024;
pub const GEMINI_MAX_META_LEN: usize = 1024;
pub const GEMINI_DEFAULT_MIME: []const u8 = "text/gemini; charset=utf-8";

// Gemini Protocol v0.16.1 Status Codes
pub const STATUS_INPUT: u8 = 10;
pub const STATUS_SENSITIVE_INPUT: u8 = 11;
pub const STATUS_SUCCESS: u8 = 20;
pub const STATUS_REDIRECT_TEMP: u8 = 30;
pub const STATUS_REDIRECT_PERM: u8 = 31;
pub const STATUS_TEMP_FAILURE: u8 = 40;
pub const STATUS_SERVER_UNAVAILABLE: u8 = 41;
pub const STATUS_CGI_ERROR: u8 = 42;
pub const STATUS_PROXY_ERROR: u8 = 43;
pub const STATUS_SLOW_DOWN: u8 = 44;
pub const STATUS_PERM_FAILURE: u8 = 50;
pub const STATUS_NOT_FOUND: u8 = 51;
pub const STATUS_GONE: u8 = 52;
pub const STATUS_PROXY_REFUSED: u8 = 53;
pub const STATUS_BAD_REQUEST: u8 = 59;
pub const STATUS_CLIENT_CERT_REQUIRED: u8 = 60;
pub const STATUS_CERT_NOT_AUTHORIZED: u8 = 61;
pub const STATUS_CERT_NOT_VALID: u8 = 62;

/// Major Gemini response status classes (tens digit 1..6).
pub enum GeminiStatusClass: u8 {
    input = 1,
    success = 2,
    redirect = 3,
    temp_failure = 4,
    perm_failure = 5,
    client_cert = 6,
}

/// Line kinds in `text/gemini` (Gemtext) documents.
pub enum GemtextLineKind: u8 {
    text = 0,
    link = 1,
    heading1 = 2,
    heading2 = 3,
    heading3 = 4,
    list_item = 5,
    quote = 6,
    preformat_toggle = 7,
    preformat_text = 8,
}

fn isAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isSchemeChar(c: u8): bool {
    return isAlpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimLeadingSpaceOrTab(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len && isSpaceOrTab(s[i])) {
        i += 1;
    }
    return s[i..];
}

fn hasMetaControlChars(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '\r' || c == '\n' || c == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn isValidGeminiUrl(url: []const u8): bool {
    if (url.len == 0 || url.len > GEMINI_MAX_URL_LEN) {
        return false;
    }
    if (!isAlpha(url[0])) {
        return false;
    }

    // Find "://" and validate scheme characters before it, plus reject control chars/spaces/'#'
    let scheme_end: usize = 0;
    let found_sep = false;
    let i: usize = 0;
    while (i < url.len) {
        const c = url[i];
        // Reject ASCII control characters, spaces, DEL, and URL fragment '#'
        if (c <= 0x20 || c == 0x7F || c == '#') {
            return false;
        }
        if (!found_sep) {
            if (c == ':' && i + 2 < url.len && url[i + 1] == '/' && url[i + 2] == '/') {
                scheme_end = i;
                found_sep = true;
                i += 3;
                continue;
            }
            if (!isSchemeChar(c)) {
                return false;
            }
        }
        i += 1;
    }

    if (!found_sep || scheme_end == 0) {
        return false;
    }
    // Must have authority/path content after "://"
    if (scheme_end + 3 >= url.len) {
        return false;
    }
    return true;
}

/// Parsed Gemini client request (`<URL>\r\n`).
pub struct GeminiRequest {
    url: []const u8,
}

/// Strictly parses a Gemini request line.
/// Requires `\r\n` termination, `url.len <= 1024`, valid `scheme://...`, no `#` fragment or control chars.
pub fn parseGeminiRequest(line: []const u8): GeminiRequest? {
    if (line.len < 2) {
        return null;
    }
    if (line[line.len - 2] != '\r' || line[line.len - 1] != '\n') {
        return null;
    }
    const url = line[0 .. line.len - 2];
    if (!isValidGeminiUrl(url)) {
        return null;
    }
    return GeminiRequest{
        .url = url,
    };
}

/// Zero-alloc encoder for a Gemini request (`<URL>\r\n`).
pub fn encodeGeminiRequest(url: []const u8, out: []u8): usize? {
    if (!isValidGeminiUrl(url)) {
        return null;
    }
    const total = url.len + 2;
    if (out.len < total) {
        return null;
    }
    let i: usize = 0;
    while (i < url.len) {
        out[i] = url[i];
        i += 1;
    }
    out[url.len] = '\r';
    out[url.len + 1] = '\n';
    return total;
}

/// Parsed Gemini response header (`<STATUS><SPACE><META>\r\n`).
pub struct GeminiResponseHeader {
    status: u8,
    meta: []const u8,

    fn statusClass(): GeminiStatusClass {
        const cls = _.status / 10;
        if (cls == 1) return GeminiStatusClass.input;
        if (cls == 2) return GeminiStatusClass.success;
        if (cls == 3) return GeminiStatusClass.redirect;
        if (cls == 4) return GeminiStatusClass.temp_failure;
        if (cls == 5) return GeminiStatusClass.perm_failure;
        return GeminiStatusClass.client_cert;
    }

    fn isInput(): bool {
        return _.status >= 10 && _.status <= 19;
    }

    fn isSuccess(): bool {
        return _.status >= 20 && _.status <= 29;
    }

    fn isRedirect(): bool {
        return _.status >= 30 && _.status <= 39;
    }

    fn isError(): bool {
        return _.status >= 40 && _.status <= 69;
    }

    /// Returns the MIME type for 2x responses (defaults to `text/gemini; charset=utf-8` when meta is empty).
    /// Returns an empty slice for non-2x responses.
    fn mimeType(): []const u8 {
        if (!_.isSuccess()) {
            return _.meta[0..0];
        }
        if (_.meta.len == 0) {
            return GEMINI_DEFAULT_MIME;
        }
        return _.meta;
    }
}

/// Strictly parses a Gemini response header line (`<d0><d1> <META>\r\n`).
/// Requires 2-digit status in `10..69`, space `0x20` at index 2, `meta.len <= 1024`, and `\r\n` termination.
pub fn parseGeminiResponseHeader(line: []const u8): GeminiResponseHeader? {
    // Minimum valid header: "20 \r\n" (5 bytes)
    if (line.len < 5) {
        return null;
    }
    if (line[line.len - 2] != '\r' || line[line.len - 1] != '\n') {
        return null;
    }
    const d0 = line[0];
    const d1 = line[1];
    if (d0 < '1' || d0 > '6' || d1 < '0' || d1 > '9') {
        return null;
    }
    if (line[2] != ' ') {
        return null;
    }
    const meta = line[3 .. line.len - 2];
    if (meta.len > GEMINI_MAX_META_LEN || hasMetaControlChars(meta)) {
        return null;
    }
    const status = (d0 - '0') * 10 + (d1 - '0');
    return GeminiResponseHeader{
        .status = status,
        .meta = meta,
    };
}

/// Zero-alloc encoder for a Gemini response header (`<STATUS> <META>\r\n`).
pub fn encodeGeminiResponseHeader(status: u8, meta: []const u8, out: []u8): usize? {
    if (status < 10 || status > 69) {
        return null;
    }
    if (meta.len > GEMINI_MAX_META_LEN || hasMetaControlChars(meta)) {
        return null;
    }
    const total = 3 + meta.len + 2;
    if (out.len < total) {
        return null;
    }
    out[0] = ('0' as u8) + (status / 10);
    out[1] = ('0' as u8) + (status % 10);
    out[2] = ' ';
    let i: usize = 0;
    while (i < meta.len) {
        out[3 + i] = meta[i];
        i += 1;
    }
    out[3 + meta.len] = '\r';
    out[4 + meta.len] = '\n';
    return total;
}

/// Single parsed line of a `text/gemini` (Gemtext) document.
pub struct GemtextLine {
    kind: GemtextLineKind,
    text: []const u8,
    url: []const u8,
    alt: []const u8,
}

/// Zero-copy line-by-line parser for `text/gemini` (Gemtext) documents.
/// Tracks `` ``` `` preformatted block state so lines inside preformatted blocks
/// are always yielded as `preformat_text` without being misinterpreted as headings or links.
pub struct GemtextIter {
    data: []const u8,
    offset: usize,
    in_preformat: bool,

    static fn init(doc: []const u8): GemtextIter {
        return GemtextIter{
            .data = doc,
            .offset = 0,
            .in_preformat = false,
        };
    }

    fn inPreformat(): bool {
        return _.in_preformat;
    }

    fn next(*_): GemtextLine? {
        if (_.offset >= _.data.len) {
            return null;
        }

        // Find end of current line (LF or EOF), stripping optional trailing CR
        let end = _.offset;
        while (end < _.data.len && _.data[end] != '\n') {
            end += 1;
        }
        let line_end = end;
        if (line_end > _.offset && _.data[line_end - 1] == '\r') {
            line_end -= 1;
        }
        const line = _.data[_.offset..line_end];
        if (end < _.data.len) {
            _.offset = end + 1;
        } else {
            _.offset = end;
        }

        const empty = line[0..0];

        // Preformatted toggle line: "```"
        if (line.len >= 3 && line[0] == '`' && line[1] == '`' && line[2] == '`') {
            const entering = !_.in_preformat;
            _.in_preformat = entering;
            const alt = if (entering) line[3..] else empty;
            return GemtextLine{
                .kind = GemtextLineKind.preformat_toggle,
                .text = empty,
                .url = empty,
                .alt = alt,
            };
        }

        // Inside preformatted block: emit raw line verbatim as preformat_text
        if (_.in_preformat) {
            return GemtextLine{
                .kind = GemtextLineKind.preformat_text,
                .text = line,
                .url = empty,
                .alt = empty,
            };
        }

        // Link line: "=> [ws] <url> [ws <label>]"
        if (line.len >= 2 && line[0] == '=' && line[1] == '>') {
            const after_arrow = trimLeadingSpaceOrTab(line[2..]);
            if (after_arrow.len > 0) {
                let url_end: usize = 0;
                while (url_end < after_arrow.len && !isSpaceOrTab(after_arrow[url_end])) {
                    url_end += 1;
                }
                const url = after_arrow[0..url_end];
                const label = trimLeadingSpaceOrTab(after_arrow[url_end..]);
                return GemtextLine{
                    .kind = GemtextLineKind.link,
                    .text = label,
                    .url = url,
                    .alt = empty,
                };
            }
        }

        // Headings: "###", "##", "#"
        if (line.len >= 3 && line[0] == '#' && line[1] == '#' && line[2] == '#') {
            return GemtextLine{
                .kind = GemtextLineKind.heading3,
                .text = trimLeadingSpaceOrTab(line[3..]),
                .url = empty,
                .alt = empty,
            };
        }
        if (line.len >= 2 && line[0] == '#' && line[1] == '#') {
            return GemtextLine{
                .kind = GemtextLineKind.heading2,
                .text = trimLeadingSpaceOrTab(line[2..]),
                .url = empty,
                .alt = empty,
            };
        }
        if (line.len >= 1 && line[0] == '#') {
            return GemtextLine{
                .kind = GemtextLineKind.heading1,
                .text = trimLeadingSpaceOrTab(line[1..]),
                .url = empty,
                .alt = empty,
            };
        }

        // Unordered list item: "* " (asterisk followed by single space)
        if (line.len >= 2 && line[0] == '*' && line[1] == ' ') {
            return GemtextLine{
                .kind = GemtextLineKind.list_item,
                .text = line[2..],
                .url = empty,
                .alt = empty,
            };
        }

        // Blockquote: ">"
        if (line.len >= 1 && line[0] == '>') {
            const q_text = if (line.len >= 2 && line[1] == ' ') line[2..] else line[1..];
            return GemtextLine{
                .kind = GemtextLineKind.quote,
                .text = q_text,
                .url = empty,
                .alt = empty,
            };
        }

        // Ordinary text line
        return GemtextLine{
            .kind = GemtextLineKind.text,
            .text = line,
            .url = empty,
            .alt = empty,
        };
    }
}
