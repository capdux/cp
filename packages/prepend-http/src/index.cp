// @cap/prepend-http — Zero-allocation URL protocol prepender for CP
// Clean-room MIT implementation with parity to npm `prepend-http` v4.

import { trim, isAlpha, isDigit, startsWith, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

fn phtIsSchemeChar(c: u8): bool {
    return isAlpha(c) || isDigit(c) || c == '+' || c == '-' || c == '.';
}

/// Returns `true` if `url` (after trimming leading/trailing whitespace) starts with
/// a protocol-relative prefix (`//`).
pub fn isProtocolRelative(url: string): bool {
    const trimmed = trim(url);
    return trimmed.len >= 2 && trimmed[0] == '/' && trimmed[1] == '/';
}

/// Returns `true` if `url` (after trimming leading/trailing whitespace) already has
/// an explicit URL scheme (`scheme://` per RFC 3986 or `mailto:`).
pub fn hasScheme(url: string): bool {
    const trimmed = trim(url);
    if (trimmed.len == 0) {
        return false;
    }
    if (trimmed.len >= 7 && eqIgnoreCase(trimmed[0..7], "mailto:")) {
        return true;
    }
    if (!isAlpha(trimmed[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < trimmed.len && phtIsSchemeChar(trimmed[i])) : (i += 1) {}
    return i + 2 < trimmed.len && trimmed[i] == ':' && trimmed[i + 1] == '/' && trimmed[i + 2] == '/';
}

/// Prepend `https://` (when `https` is `true`) or `http://` (when `https` is `false`)
/// to `url` if it is missing a scheme and is not protocol-relative or a relative path
/// (`scheme://`, `//`, `mailto:`, `./`, `../`, `/`).
/// Trims leading/trailing whitespace, writes the result into `out`, and returns bytes written.
pub fn prependHttp(url: string, https: bool, out: []u8): usize {
    const trimmed = trim(url);
    if (trimmed.len == 0) {
        return 0;
    }
    const keep_as_is =
        hasScheme(trimmed) ||
        isProtocolRelative(trimmed) ||
        startsWith(trimmed, "./") ||
        startsWith(trimmed, "../") ||
        startsWith(trimmed, "/");

    if (keep_as_is) {
        return writeStr(trimmed, out, 0);
    }

    const prefix = if (https) "https://" else "http://";
    const pos = writeStr(prefix, out, 0);
    return writeStr(trimmed, out, pos);
}
