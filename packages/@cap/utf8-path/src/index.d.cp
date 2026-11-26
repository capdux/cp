// Type definitions for CP

export fn isSep(c: u8): bool;
export fn isAsciiAlpha(c: u8): bool;
export fn strEq(a: string, b: string): bool;
export fn hasDrivePrefix(p: string): bool;
export fn isAbsolute(p: string): bool;
export fn trimTrailingSeps(p: string): string;
export fn fileName(p: string): string?;
export fn fileStem(p: string): string?;
export fn extension(p: string): string?;
export fn parentDir(p: string): string?;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export class SplitResult {
    isAbs: bool,
    hasDrive: bool,
    driveLetter: u8,
    segs: [MAX_SEGS]string,
    count: usize,
}

export fn splitAndClean(p: string): SplitResult;
export fn normalizePath(p: string, out: []u8): string;
export fn joinPath(base: string, rel: string, out: []u8): string;
export fn relEscapesRoot(rel: string): bool;
export fn safeJoin(root: string, untrusted_rel: string, out: []u8): string?;
export fn relativePath(from_dir: string, to_path: string, out: []u8): string;
export fn withExtension(p: string, new_ext: string, out: []u8): string;
