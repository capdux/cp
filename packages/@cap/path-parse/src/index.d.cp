// Type definitions for CP

export class ParsedPath {
    root: string,
    dir: string,
    base: string,
    ext: string,
    name: string,
}

export fn pprIsWinSep(b: u8): bool;
export fn pprIsAlpha(b: u8): bool;
export class PprNameExt {
    name: string,
    ext: string,
}

export fn pprSplitBaseExt(base: string): PprNameExt;
export fn parsePosix(path: string): ParsedPath;
export fn parseWin32(path: string): ParsedPath;
