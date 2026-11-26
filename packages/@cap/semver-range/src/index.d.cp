// Type definitions for CP

export fn svrIsSpace(c: u8): bool;
export fn svrIsDigit(c: u8): bool;
export fn svrIsIdentChar(c: u8): bool;
export fn svrIsWildcardStr(s: string): bool;
export fn svrTrimBounds(s: string, out_start: &mut usize, out_end: &mut usize);
export fn svrParseU32Strict(s: string, out_val: &mut u32): bool;
export fn svrIsAllDigits(s: &[]u8): bool;
export fn svrValidatePre(s: string): bool;
export fn svrValidateBuild(s: string): bool;
export fn svrCopyBytes(src: string, dst: []u8): usize;
export fn svrAppendU32(out: []u8, pos: &mut usize, val: u32);
export fn svrAppendSlice(out: []u8, pos: &mut usize, src: &[]u8);
export fn svrAppendByte(out: []u8, pos: &mut usize, b: u8);
export class SemVer {
    major: u32,
    minor: u32,
    patch: u32,
    pre: [32]u8,
    pre_len: usize,
    build: [32]u8,
    build_len: usize,
    valid: bool,
    fn invalid(): SemVer;
    fn make(major: u32, minor: u32, patch: u32): SemVer;
    fn preStr(): &[]u8;
    fn buildStr(): &[]u8;
}

export fn parseSemver(s: string): SemVer;
export fn svrComparePreIdent(a: &[]u8, b: &[]u8): i32;
export fn compareSemver(a: SemVer, b: SemVer): i32;
export class SvrPartial {
    major: u32,
    minor: u32,
    patch: u32,
    has_major: bool,
    has_minor: bool,
    has_patch: bool,
    pre: [32]u8,
    pre_len: usize,
    valid: bool,
}

export fn svrParsePartial(raw: string): SvrPartial;
export fn svrPartialToSemver(p: SvrPartial): SemVer;
export fn svrSameTuplePre(v: SemVer, p: SvrPartial): bool;
export fn svrMatchSingleComparator(v: SemVer, tok: string, allow_pre: &mut bool): bool;
export fn svrIsBareOperator(tok: string): bool;
export fn svrMatchClause(v: SemVer, clause: string): bool;
export fn satisfies(version: string, range: string): bool;
export fn maxSatisfying(versions: &[]string, range: string): i32;
export fn minSatisfying(versions: &[]string, range: string): i32;
export fn incMajor(v: SemVer): SemVer;
export fn incMinor(v: SemVer): SemVer;
export fn incPatch(v: SemVer): SemVer;
export fn formatSemver(v: SemVer, out: []u8): usize;
