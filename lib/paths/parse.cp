// Source: bun/src/paths/resolve_path_.zig -> cp/lib/paths/parse.cp (Zero-Allocation Path Parsing & Slash Conversion)
// Pure CP zero-allocation path decomposition (parse, dirname, basename, extname) and slash conversion.

const std = @import("std");
import {
    PathError,
    PathStyle,
    ParsedPath,
    isSep,
    rootLen,
    containsNullByte,
} from "./types.cp";

/// Decomposes `path` into zero-allocation slice views (`root`, `dir`, `base`, `ext`, `name`).
pub fn parse(path: string, style: PathStyle): ParsedPath {
    if (path.len == 0) {
        return ParsedPath{
            root: "",
            dir: "",
            base: "",
            ext: "",
            name: "",
        };
    }

    const rlen = rootLen(path, style);
    const root = path[0..rlen];

    // Strip trailing separators past root
    let end = path.len;
    while (end > rlen && isSep(path[end - 1], style)) {
        end -= 1;
    }

    if (end <= rlen) {
        return ParsedPath{
            root: root,
            dir: root,
            base: "",
            ext: "",
            name: "",
        };
    }

    // Find last separator in [rlen .. end)
    let sep_idx: usize = end;
    let found_sep = false;
    let i: usize = end;
    while (i > rlen) {
        i -= 1;
        if (isSep(path[i], style)) {
            sep_idx = i;
            found_sep = true;
            break;
        }
    }

    let dir: string = root;
    let base: string = path[rlen..end];
    if (found_sep) {
        base = path[(sep_idx + 1)..end];
        let dir_end = sep_idx;
        while (dir_end > rlen && isSep(path[dir_end - 1], style)) {
            dir_end -= 1;
        }
        dir = path[0..dir_end];
    }

    // Extract extension and name from `base`
    let ext: string = "";
    let name: string = base;
    if (base.len > 0 && !std.mem.eql(u8, base, ".") && !std.mem.eql(u8, base, "..")) {
        let j: usize = base.len;
        while (j > 0) {
            j -= 1;
            if (base[j] == '.') {
                if (j > 0) {
                    ext = base[j..base.len];
                    name = base[0..j];
                }
                break;
            }
        }
    }

    return ParsedPath{
        root: root,
        dir: dir,
        base: base,
        ext: ext,
        name: name,
    };
}

/// Returns the directory portion of `path` as a zero-allocation slice view.
pub fn dirname(path: string, style: PathStyle): string {
    return parse(path, style).dir;
}

/// Returns the final path component of `path` as a zero-allocation slice view.
pub fn basename(path: string, style: PathStyle): string {
    return parse(path, style).base;
}

/// Returns the extension (including leading `.`) of `path` as a zero-allocation slice view.
pub fn extname(path: string, style: PathStyle): string {
    return parse(path, style).ext;
}

/// Converts all `\` separators in `path` to `/`. Caller owns returned slice.
pub fn toPosixSlash(alloc: std.mem.Allocator, path: string): string ! PathError {
    if (containsNullByte(path)) {
        return PathError.NullByteFound;
    }
    let out = alloc.alloc(u8, path.len) catch return PathError.OutOfMemory;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == '\\') {
            out[i] = '/';
        } else {
            out[i] = path[i];
        }
        i += 1;
    }
    return out;
}

/// Converts all `/` separators in `path` to `\`. Caller owns returned slice.
pub fn toWindowsSlash(alloc: std.mem.Allocator, path: string): string ! PathError {
    if (containsNullByte(path)) {
        return PathError.NullByteFound;
    }
    let out = alloc.alloc(u8, path.len) catch return PathError.OutOfMemory;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == '/') {
            out[i] = '\\';
        } else {
            out[i] = path[i];
        }
        i += 1;
    }
    return out;
}
