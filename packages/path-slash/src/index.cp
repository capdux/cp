// @cap/path-slash — Zero-heap POSIX '/' and Windows '\' path conversion
// Clean-room pure CP counterpart to crates.io:path-slash

import { findByteInBytes } from "cap:bin";
import { writeStr } from "cap:fmt";

fn pshIsAsciiAlpha(b: u8): bool {
    return (b >= 0x41 && b <= 0x5A) || (b >= 0x61 && b <= 0x7A);
}

fn pshIsSep(b: u8): bool {
    return b == 0x2F || b == 0x5C;
}

fn pshWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pshValidUtf8Len(s: string, offset: usize): usize {
    if (offset >= s.len) {
        return 0;
    }
    const b0 = s[offset] as u32;
    if (b0 < 0x80) {
        return 1;
    }
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (offset + 1 >= s.len) {
            return 0;
        }
        const b1 = s[offset + 1] as u32;
        if ((b1 & 0xC0) != 0x80) {
            return 0;
        }
        return 2;
    }
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (offset + 2 >= s.len) {
            return 0;
        }
        const b1 = s[offset + 1] as u32;
        const b2 = s[offset + 2] as u32;
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
            return 0;
        }
        const cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return 0;
        }
        return 3;
    }
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (offset + 3 >= s.len) {
            return 0;
        }
        const b1 = s[offset + 1] as u32;
        const b2 = s[offset + 2] as u32;
        const b3 = s[offset + 3] as u32;
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
            return 0;
        }
        const cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) {
            return 0;
        }
        return 4;
    }
    return 0;
}

fn pshIsAbs(path: string): bool {
    if (path.len == 0) {
        return false;
    }
    if (pshIsSep(path[0])) {
        return true;
    }
    if (path.len >= 2 && pshIsAsciiAlpha(path[0]) && path[1] == 0x3A) {
        return true;
    }
    return false;
}

/// Convert all Windows backslashes `\` in `path` to POSIX forward slashes `/`.
/// Preserves UNC (`\\server\share` -> `//server/share`) and drive letters (`C:\a` -> `C:/a`).
pub fn toSlash(path: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < path.len && pos < out.len) {
        const b = path[i];
        out[pos] = if (b == 0x5C) 0x2F else b;
        pos += 1;
        i += 1;
    }
    return pos;
}

/// Convert `path` to slash-separated UTF-8, replacing invalid UTF-8 bytes with U+FFFD.
pub fn toSlashLossy(path: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < path.len && pos < out.len) {
        const clen = pshValidUtf8Len(path, i);
        if (clen == 0) {
            pos = writeStr("\xEF\xBF\xBD", out, pos);
            i += 1;
        } else if (clen == 1) {
            const b = path[i];
            pos = pshWriteByte(out, pos, if (b == 0x5C) 0x2F else b);
            i += 1;
        } else {
            pos = writeStr(path[i..i + clen], out, pos);
            i += clen;
        }
    }
    return pos;
}

/// Convert all POSIX forward slashes `/` in `path` to Windows backslashes `\`.
/// Preserves UNC (`//server/share` -> `\\server\share`) and drive letters (`C:/a` -> `C:\a`).
pub fn fromSlash(path: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < path.len && pos < out.len) {
        const b = path[i];
        out[pos] = if (b == 0x2F) 0x5C else b;
        pos += 1;
        i += 1;
    }
    return pos;
}

/// Returns true if `path` contains at least one backslash `\`.
pub fn hasBackslash(path: string): bool {
    return findByteInBytes(path, 0x5C, 0) != null;
}

/// Returns true if `path` contains at least one forward slash `/`.
pub fn hasForwardSlash(path: string): bool {
    return findByteInBytes(path, 0x2F, 0) != null;
}

/// Trim trailing `/` or `\` separators while preserving root paths (`/`, `\`, `C:/`, `C:\`).
pub fn trimTrailingSlashes(path: string): string {
    if (path.len <= 1) {
        return path;
    }
    let end = path.len;
    while (end > 1 && pshIsSep(path[end - 1])) {
        if (end == 3 && pshIsAsciiAlpha(path[0]) && path[1] == 0x3A) {
            break;
        }
        end -= 1;
    }
    return path[0..end];
}

/// Join `a` and `b` using POSIX `/` separators (converting any `\` in either input to `/`).
pub fn joinSlash(a: string, b: string, out: []u8): usize {
    if (b.len == 0) {
        return toSlash(a, out);
    }
    if (a.len == 0 || pshIsAbs(b)) {
        return toSlash(b, out);
    }

    let aEnd = a.len;
    while (aEnd > 1 && pshIsSep(a[aEnd - 1])) {
        if (aEnd == 3 && pshIsAsciiAlpha(a[0]) && a[1] == 0x3A) {
            break;
        }
        if (aEnd == 2 && pshIsSep(a[0]) && pshIsSep(a[1])) {
            break;
        }
        aEnd -= 1;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < aEnd && pos < out.len) {
        const ch = a[i];
        out[pos] = if (ch == 0x5C) 0x2F else ch;
        pos += 1;
        i += 1;
    }

    if (pos > 0 && out[pos - 1] != 0x2F) {
        pos = pshWriteByte(out, pos, 0x2F);
    }

    let j: usize = 0;
    while (j < b.len && pos < out.len) {
        const ch = b[j];
        out[pos] = if (ch == 0x5C) 0x2F else ch;
        pos += 1;
        j += 1;
    }
    return pos;
}
