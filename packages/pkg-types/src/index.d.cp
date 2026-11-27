// Type definitions for CP

export class PkgManifest {
    name: string,
    version: string,
    license: string,
    main: string,
    module: string,
    entry: string,
    is_private: bool,
    valid: bool,
}

export class PkgNameParts {
    scope: string,
    pkg_name: string,
    has_scope: bool,
}

export fn pkgtIsSpace(c: u8): bool;
export fn pkgtSkipSpace(s: string, start: usize): usize;
export fn pkgtStrEq(a: string, b: string): bool;
export fn pkgtScanStringEnd(s: string, open_quote: usize): usize;
export fn pkgtSkipValue(s: string, start: usize): usize;
export fn pkgtEmptyManifest(): PkgManifest;
export fn parsePkgManifest(json_text: string): PkgManifest;
export fn pkgtIsValidSegmentChar(c: u8): bool;
export fn pkgtIsValidSegment(seg: string): bool;
export fn isValidPkgName(name: string): bool;
export fn splitPkgScope(name: string): PkgNameParts;
