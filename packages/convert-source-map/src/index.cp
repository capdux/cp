// @cap/convert-source-map — Zero-allocation inline sourceMappingURL parser & generator in pure CP
// Clean-room MIT implementation of npm `convert-source-map` (#96, 307M/wk).

pub enum SourceMapCommentKind {
    None,
    InlineBase64,
    ExternalFile,
}

pub struct SourceMapComment {
    pub kind: SourceMapCommentKind,
    pub comment_start: usize,
    pub comment_end: usize,
    pub payload: string, // base64 data or external file path
}

fn csmStartsWith(s: string, pos: usize, lit: string): bool {
    if (pos + lit.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        if (s[pos + i] != lit[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn csmPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn csmB64Val(b: u8): i32 {
    if (b >= 65 && b <= 90) { return (b - 65) as i32; }
    if (b >= 97 && b <= 122) { return (b - 97 + 26) as i32; }
    if (b >= 48 && b <= 57) { return (b - 48 + 52) as i32; }
    if (b == 43) { return 62; }
    if (b == 47) { return 63; }
    return -1;
}

fn csmB64Encode(data: []const u8, out: []u8, start_pos: usize): usize {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let p = start_pos;
    let i: usize = 0;
    while (i < data.len && p + 4 <= out.len) {
        const b0: u32 = data[i] as u32;
        const has1 = i + 1 < data.len;
        const has2 = i + 2 < data.len;
        let b1: u32 = 0;
        let b2: u32 = 0;
        if (has1) { b1 = data[i + 1] as u32; }
        if (has2) { b2 = data[i + 2] as u32; }
        const triple: u32 = (b0 << 16) | (b1 << 8) | b2;
        out[p] = alphabet[((triple >> 18) & 0x3F) as usize];
        out[p + 1] = alphabet[((triple >> 12) & 0x3F) as usize];
        if (has1) {
            out[p + 2] = alphabet[((triple >> 6) & 0x3F) as usize];
        } else {
            out[p + 2] = 61; // '='
        }
        if (has2) {
            out[p + 3] = alphabet[(triple & 0x3F) as usize];
        } else {
            out[p + 3] = 61;
        }
        p += 4;
        i += 3;
    }
    return p;
}

fn csmB64Decode(b64: string, out: []u8): i64 {
    let p: usize = 0;
    let i: usize = 0;
    while (i < b64.len) {
        while (i < b64.len && (b64[i] == 32 || b64[i] == 9 || b64[i] == 10 || b64[i] == 13)) {
            i += 1;
        }
        if (i >= b64.len) {
            break;
        }
        if (i + 3 >= b64.len) {
            return -1;
        }
        const v0 = csmB64Val(b64[i]);
        const v1 = csmB64Val(b64[i + 1]);
        if (v0 < 0 || v1 < 0) {
            return -1;
        }
        const pad2 = b64[i + 2] == 61;
        const pad3 = b64[i + 3] == 61;
        let v2: i32 = 0;
        let v3: i32 = 0;
        if (!pad2) {
            v2 = csmB64Val(b64[i + 2]);
            if (v2 < 0) { return -1; }
        }
        if (!pad3) {
            v3 = csmB64Val(b64[i + 3]);
            if (v3 < 0) { return -1; }
        }
        const triple: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6) | (v3 as u32);
        if (p < out.len) { out[p] = ((triple >> 16) & 0xFF) as u8; p += 1; }
        if (!pad2 && p < out.len) { out[p] = ((triple >> 8) & 0xFF) as u8; p += 1; }
        if (!pad3 && p < out.len) { out[p] = (triple & 0xFF) as u8; p += 1; }
        i += 4;
    }
    return p as i64;
}

/// Finds the last `//# sourceMappingURL=...` or `/*# sourceMappingURL=... */` comment in `src`.
pub fn findSourceMapComment(src: string): SourceMapComment {
    let last = SourceMapComment { kind: .None, comment_start: 0, comment_end: 0, payload: "" };
    let i: usize = 0;
    while (i < src.len) {
        let is_line = false;
        let is_block = false;
        if (csmStartsWith(src, i, "//#") || csmStartsWith(src, i, "//@")) {
            is_line = true;
        } else if (csmStartsWith(src, i, "/*#") || csmStartsWith(src, i, "/*@")) {
            is_block = true;
        }
        if (is_line || is_block) {
            let p = i + 3;
            while (p < src.len && (src[p] == 32 || src[p] == 9)) {
                p += 1;
            }
            if (csmStartsWith(src, p, "sourceMappingURL=")) {
                const url_start = p + 17;
                let url_end = url_start;
                let c_end = url_start;
                if (is_line) {
                    while (url_end < src.len && src[url_end] != 10 && src[url_end] != 13) {
                        url_end += 1;
                    }
                    c_end = url_end;
                    while (url_end > url_start && (src[url_end - 1] == 32 || src[url_end - 1] == 9)) {
                        url_end -= 1;
                    }
                } else {
                    while (url_end + 1 < src.len && !(src[url_end] == 42 && src[url_end + 1] == 47)) {
                        url_end += 1;
                    }
                    if (url_end + 1 < src.len && src[url_end] == 42 && src[url_end + 1] == 47) {
                        c_end = url_end + 2;
                        while (url_end > url_start && (src[url_end - 1] == 32 || src[url_end - 1] == 9)) {
                            url_end -= 1;
                        }
                    } else {
                        i += 1;
                        continue;
                    }
                }
                const url = src[url_start..url_end];
                // Check for inline data:application/json;...;base64,
                let b64_idx: usize = url.len;
                let k: usize = 0;
                while (k + 7 <= url.len) {
                    if (csmStartsWith(url, k, "base64,")) {
                        b64_idx = k + 7;
                        break;
                    }
                    k += 1;
                }
                if (csmStartsWith(url, 0, "data:application/json") && b64_idx < url.len) {
                    last = SourceMapComment {
                        kind: .InlineBase64,
                        comment_start: i,
                        comment_end: c_end,
                        payload: url[b64_idx..url.len],
                    };
                } else {
                    last = SourceMapComment {
                        kind: .ExternalFile,
                        comment_start: i,
                        comment_end: c_end,
                        payload: url,
                    };
                }
                i = c_end;
                continue;
            }
        }
        i += 1;
    }
    return last;
}

/// Decodes an inline Base64 `sourceMappingURL` JSON payload from `src` into `out`.
/// Returns decoded byte length, or `-1` if no valid inline Base64 source map is present.
pub fn fromSource(src: string, out: []u8): i64 {
    const c = findSourceMapComment(src);
    if (c.kind != .InlineBase64) {
        return -1;
    }
    return csmB64Decode(c.payload, out);
}

/// Encodes `json_map` into a JS `//# sourceMappingURL=data:application/json;charset=utf-8;base64,...` comment.
pub fn toComment(json_map: string, multiline: bool, out: []u8): usize {
    let p: usize = 0;
    if (multiline) {
        p = csmPush(out, p, "/*# sourceMappingURL=data:application/json;charset=utf-8;base64,");
    } else {
        p = csmPush(out, p, "//# sourceMappingURL=data:application/json;charset=utf-8;base64,");
    }
    p = csmB64Encode(json_map, out, p);
    if (multiline) {
        p = csmPush(out, p, " */");
    }
    return p;
}

/// Strips the `sourceMappingURL` comment from `src` into `out`.
pub fn removeComments(src: string, out: []u8): usize {
    const c = findSourceMapComment(src);
    if (c.kind == .None) {
        return csmPush(out, 0, src);
    }
    let p = csmPush(out, 0, src[0..c.comment_start]);
    return csmPush(out, p, src[c.comment_end..src.len]);
}

export {
    SourceMapCommentKind,
    SourceMapComment,
    findSourceMapComment,
    fromSource,
    toComment,
    removeComments,
};
