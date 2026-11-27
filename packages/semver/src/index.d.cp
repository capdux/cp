// Type definitions for CP

import { isSpace, isDigit, trim, strEq } from "cap:ascii";
import { writeU64, writeStr } from "cap:fmt";
export fn svrIsIdentChar(c: u8): bool;
export fn svrIsWildcardStr(s: string): bool;
export fn svrParseU32Strict(s: string): u32?;
export fn svrIsAllDigits(s: &[]u8): bool;
export fn svrValidatePre(s: string): bool;
export fn svrValidateBuild(s: string): bool;
export fn svrCopyBytes(src: string, dst: []u8): usize;
export fn svrWriteByte(out: []u8, pos: usize, b: u8): usize;
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
    fn make(maj: u32, min: u32, pat: u32): SemVer;
    fn parse(s: string): SemVer;
    fn preStr(): &[]u8;
    fn buildStr(): &[]u8;
    fn isPrerelease(): bool;
    fn writeTo(out: []u8): usize;
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
export class SvrCompResult {
    matched: bool,
    allow_pre: bool,
}

export fn svrMatchSingleComparator(v: SemVer, tok: string): SvrCompResult;
export fn svrIsBareOperator(tok: string): bool;
export fn svrMatchClause(v: SemVer, clause: string): bool;
export fn svrSatisfiesVer(v: SemVer, range: string): bool;
export fn svrValidateReqPart(part: string): bool;
export fn svrMatchesVersionReq(v: SemVer, req: string): bool;
export class VersionReq {
    raw: string,
    valid: bool,
    fn parse(req: string): VersionReq;
    fn matches(v: Version): bool;
    fn matchesStr(version_str: string): bool;
}

export fn parseVersion(s: string): Version;
export fn compareVersions(a: Version, b: Version): i32;
export fn isPrerelease(v: Version): bool;
export fn matchesVersionReq(version_str: string, req: string): bool;
export fn isCaretCompatible(ver: string, target: string): bool;
export fn formatVersion(v: Version, out: []u8): usize;
export fn bumpMajor(v: Version): Version;
export fn bumpMinor(v: Version): Version;
export fn bumpPatch(v: Version): Version;
export fn satisfies(version: string, range: string): bool;
export fn valid(s: string): bool;
export fn compare(a: string, b: string): i32;
export fn gt(a: string, b: string): bool;
export fn gte(a: string, b: string): bool;
export fn lt(a: string, b: string): bool;
export fn lte(a: string, b: string): bool;
export fn eq(a: string, b: string): bool;
export fn neq(a: string, b: string): bool;
export fn major(s: string): u32?;
export fn minor(s: string): u32?;
export fn patch(s: string): u32?;
export fn clean(version: string, out: []u8): usize;
export fn inc(version: string, release: string, out: []u8): usize;
export fn maxSatisfying(versions: &[]string, range: string): i32;
export fn minSatisfying(versions: &[]string, range: string): i32;
export fn incMajor(v: SemVer): SemVer;
export fn incMinor(v: SemVer): SemVer;
export fn incPatch(v: SemVer): SemVer;
export fn formatSemver(v: SemVer, out: []u8): usize;
