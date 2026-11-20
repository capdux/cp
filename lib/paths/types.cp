// Source: bun/src/paths/paths.cp -> cp/lib/paths/types.cp (Path Types, Errors & Primitives)
// Pure CP cross-platform path types, error definitions, and zero-allocation predicates.

const std = @import("std");

/// Error set for path parsing, normalization, and boundary validation.
pub err PathError {
    InvalidPath,
    NullByteFound,
    PathTraversal,
    InvalidDriveLetter,
    OutOfMemory,
}

/// Target path semantics style.
pub enum PathStyle {
    Posix,
    Windows,
}

/// Zero-allocation slice view representing a decomposed path.
pub struct ParsedPath {
    root: string,
    dir: string,
    base: string,
    ext: string,
    name: string,
}

/// Returns true if `c` is an ASCII letter (`A-Z` or `a-z`).
pub fn isDriveLetter(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/// Returns ASCII lowercase of `c`.
pub fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

/// Case-insensitive ASCII slice equality.
pub fn eqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Returns the canonical separator character for `style`.
pub fn sepChar(style: PathStyle): u8 {
    if (style == .Windows) {
        return '\\';
    }
    return '/';
}

/// Returns true if `c` is a path separator under `style`.
pub fn isSep(c: u8, style: PathStyle): bool {
    if (style == .Windows) {
        return c == '/' || c == '\\';
    }
    return c == '/';
}

/// Returns true if `path` contains an embedded NUL byte (`0x00`).
pub fn containsNullByte(path: string): bool {
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Computes the byte length of the Windows UNC volume (`\\server\share`) without trailing slash,
/// or 0 if `path` is not a UNC path.
pub fn windowsUncVolumeLen(path: string): usize {
    if (path.len < 5) {
        return 0;
    }
    if (!isSep(path[0], .Windows) || !isSep(path[1], .Windows)) {
        return 0;
    }
    if (isSep(path[2], .Windows) || path[2] == '.') {
        return 0;
    }
    // Find end of server name
    let i: usize = 3;
    while (i < path.len && !isSep(path[i], .Windows)) {
        i += 1;
    }
    if (i >= path.len) {
        return 0;
    }
    // Must have at least one non-separator char for share name
    if (i + 1 >= path.len || isSep(path[i + 1], .Windows)) {
        return 0;
    }
    let j: usize = i + 1;
    while (j < path.len && !isSep(path[j], .Windows)) {
        j += 1;
    }
    return j;
}

/// Computes the root prefix byte length under `style`.
/// Examples:
/// - Posix: "/" -> 1, "///a" -> 1, "a/b" -> 0
/// - Windows: "C:\\a" -> 3, "C:a" -> 2, "\\\\srv\\shr\\a" -> 10, "\\\\srv\\shr" -> 9, "\\a" -> 1
pub fn rootLen(path: string, style: PathStyle): usize {
    if (path.len == 0) {
        return 0;
    }
    if (style == .Posix) {
        if (path[0] == '/') {
            return 1;
        }
        return 0;
    }

    // Windows UNC: \\server\share\ or \\server\share
    const unc_vol = windowsUncVolumeLen(path);
    if (unc_vol > 0) {
        if (unc_vol < path.len && isSep(path[unc_vol], .Windows)) {
            return unc_vol + 1;
        }
        return unc_vol;
    }

    // Windows leading double sep without valid UNC: treat root as 2 or 1
    if (path.len >= 2 && isSep(path[0], .Windows) && isSep(path[1], .Windows)) {
        return 1;
    }

    // Windows drive letter: C:\ or C:
    if (path.len >= 2 && isDriveLetter(path[0]) && path[1] == ':') {
        if (path.len >= 3 && isSep(path[2], .Windows)) {
            return 3;
        }
        return 2;
    }

    // Windows single leading separator: \foo or /foo
    if (isSep(path[0], .Windows)) {
        return 1;
    }

    return 0;
}

/// Returns true if `path` is an absolute path under `style`.
pub fn isAbsolute(path: string, style: PathStyle): bool {
    if (path.len == 0) {
        return false;
    }
    if (style == .Posix) {
        return path[0] == '/';
    }

    // Windows: UNC is absolute, leading \ or / is absolute, C:\ or C:/ is absolute (C:foo is NOT)
    if (windowsUncVolumeLen(path) > 0) {
        return true;
    }
    if (isSep(path[0], .Windows)) {
        return true;
    }
    if (path.len >= 3 && isDriveLetter(path[0]) && path[1] == ':' && isSep(path[2], .Windows)) {
        return true;
    }
    return false;
}

/// Returns true if `path` ends with a separator under `style`.
pub fn hasTrailingSlash(path: string, style: PathStyle): bool {
    if (path.len == 0) {
        return false;
    }
    return isSep(path[path.len - 1], style);
}

/// Trims trailing separators while preserving the root prefix (e.g. "/", "C:\\", "\\\\srv\\shr\\").
pub fn trimTrailingSlashes(path: string, style: PathStyle): string {
    if (path.len == 0) {
        return path;
    }
    const rlen = rootLen(path, style);
    let end = path.len;
    while (end > rlen && isSep(path[end - 1], style)) {
        end -= 1;
    }
    return path[0..end];
}
