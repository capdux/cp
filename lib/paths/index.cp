// Source: cp/lib/paths/index.cp (Unified Exports for cap:paths)
// Pure CP Cross-Platform Path Normalization, Parsing & Security Boundary Library (`cap:paths` & `cap.paths`).

const std = @import("std");
import * as ops from "./ops.cp";

export {
    PathError,
    PathStyle,
    ParsedPath,
    isDriveLetter,
    containsNullByte,
    isAbsolute,
    hasTrailingSlash,
    trimTrailingSlashes,
} from "./types.cp";

export {
    parse,
    dirname,
    basename,
    extname,
    toPosixSlash,
    toWindowsSlash,
} from "./parse.cp";

export {
    normalize,
    resolve,
    relative,
} from "./ops.cp";

export {
    isSubpath,
    safeJoin,
    matchGlobSimple,
} from "./security.cp";

/// Joins `parts` with the style separator and normalizes the resulting path.
pub fn join(alloc: std.mem.Allocator, parts: []const string, style: PathStyle): string ! PathError {
    return ops.join(alloc, parts, style);
}
