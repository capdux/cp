// Type definitions for CP

export class SemVer {
    major: u32,
    minor: u32,
    patch: u32,
    valid: bool,
}

export fn srIsSpace(c: u8): bool;
export fn srTrim(s: &[]u8): &[]u8;
export fn srIsWildcardToken(s: &[]u8): bool;
export fn srParseU32(s: &[]u8, out: &mut u32): bool;
export fn parseVersion(s: string): SemVer;
export fn compareVersion(a: SemVer, b: SemVer): i32;
export fn srEvalComparator(ver: SemVer, rawComp: &[]u8, evalVer: bool): i32;
export fn srIsOpOnly(tok: &[]u8): bool;
export fn srEvalBranch(ver: SemVer, branch: &[]u8, evalVer: bool): i32;
export fn srEvalRange(ver: SemVer, range: string, evalVer: bool): i32;
export fn isValidRange(range: string): bool;
export fn satisfies(version: string, range: string): bool;
export fn maxSatisfyingIndex(versions: &[]string, range: string): i64;
