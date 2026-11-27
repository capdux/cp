// @cap/ufo — Zero-heap URL path normalization and joining utilities in pure CP.

fn ufoIsAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn ufoIsSchemeChar(c: u8): bool {
    return ufoIsAlpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
}

// Returns the byte index immediately after "scheme://", or 0 if no protocol is present.
fn ufoProtocolEnd(url: string): usize {
    if (url.len < 4 || !ufoIsAlpha(url[0])) {
        return 0;
    }
    let i: usize = 1;
    while (i < url.len && ufoIsSchemeChar(url[i])) : (i += 1) {}
    if (i + 2 < url.len && url[i] == ':' && url[i + 1] == '/' && url[i + 2] == '/') {
        return i + 3;
    }
    return 0;
}

fn ufoCopyBytes(src: string, out: []u8, offset: usize): usize {
    let written = offset;
    let i: usize = 0;
    while (i < src.len && written < out.len) : (i += 1) {
        out[written] = src[i];
        written += 1;
    }
    return written;
}

pub fn hasProtocol(url: string): bool {
    return ufoProtocolEnd(url) > 0;
}

pub fn isRelative(url: string): bool {
    if (url.len == 0) {
        return false;
    }
    if (url[0] == '/') {
        return false;
    }
    return !hasProtocol(url);
}

pub fn withoutLeadingSlash(s: string): string {
    let start: usize = 0;
    while (start < s.len && s[start] == '/') : (start += 1) {}
    return s[start..s.len];
}

pub fn withoutTrailingSlash(s: string): string {
    let end: usize = s.len;
    while (end > 0 && s[end - 1] == '/') : (end -= 1) {}
    return s[0..end];
}

pub fn withLeadingSlash(s: string, out: []u8): usize {
    if (s.len > 0 && s[0] == '/') {
        return ufoCopyBytes(s, out, 0);
    }
    let pos: usize = 0;
    if (pos < out.len) {
        out[pos] = '/';
        pos += 1;
    }
    return ufoCopyBytes(s, out, pos);
}

pub fn withTrailingSlash(s: string, out: []u8): usize {
    if (s.len > 0 && s[s.len - 1] == '/') {
        return ufoCopyBytes(s, out, 0);
    }
    let pos = ufoCopyBytes(s, out, 0);
    if (pos < out.len) {
        out[pos] = '/';
        pos += 1;
    }
    return pos;
}

pub fn cleanDoubleSlashes(url: string, out: []u8): usize {
    const protoEnd = ufoProtocolEnd(url);
    let pos: usize = 0;
    let i: usize = 0;
    while (i < protoEnd && pos < out.len) : (i += 1) {
        out[pos] = url[i];
        pos += 1;
    }
    let prevSlash = false;
    while (i < url.len && pos < out.len) : (i += 1) {
        const c = url[i];
        if (c == '/') {
            if (!prevSlash) {
                out[pos] = '/';
                pos += 1;
                prevSlash = true;
            }
        } else {
            out[pos] = c;
            pos += 1;
            prevSlash = false;
        }
    }
    return pos;
}

pub fn joinUrl(base: string, segment: string, out: []u8): usize {
    if (base.len == 0) {
        return ufoCopyBytes(segment, out, 0);
    }
    if (segment.len == 0) {
        return ufoCopyBytes(base, out, 0);
    }

    const protoEnd = ufoProtocolEnd(base);
    let baseEnd: usize = base.len;
    while (baseEnd > protoEnd && base[baseEnd - 1] == '/') : (baseEnd -= 1) {}
    const basePart = base[0..baseEnd];
    const segPart = withoutLeadingSlash(segment);

    let pos = ufoCopyBytes(basePart, out, 0);
    if (segPart.len == 0) {
        if (pos == 0 || (pos < out.len && out[pos - 1] != '/')) {
            if (pos < out.len) {
                out[pos] = '/';
                pos += 1;
            }
        }
        return pos;
    }

    if (pos == 0 || out[pos - 1] != '/') {
        if (pos < out.len) {
            out[pos] = '/';
            pos += 1;
        }
    }
    return ufoCopyBytes(segPart, out, pos);
}
