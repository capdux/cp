// Type definitions for CP

export fn npIsSlash(ch: u8): bool;
export fn npIsAsciiAlpha(ch: u8): bool;
export fn hasBackslash(path: string): bool;
export fn isUncPath(path: string): bool;
export fn normalizePath(path: string, strip_trailing: bool, out: []u8): usize;
export fn normalizePathDefault(path: string, out: []u8): usize;
export class NormalizePathOptions {
    strip_trailing: bool,
    resolve_dots: bool,
    fn defaults(): NormalizePathOptions;
}

export fn isWindowsDrivePath(path: string): bool;
export fn toPosixPath(path: string, out: []u8): usize;
export fn stripTrailingSlash(path: string): string;
export fn normalizePathWith(path: string, opts: NormalizePathOptions, out: []u8): usize;
