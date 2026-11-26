// Type definitions for CP

import { SemVer, parseSemver, compareSemver, satisfies, incMajor, incMinor, incPatch, formatSemver } from "../../semver-range/src/index.cp";
export fn smcIsWs(b: u8): bool;
export fn smcTrim(s: string): string;
export fn parseVersion(s: string): SemVer;
export fn compareVersions(a: SemVer, b: SemVer): i32;
export fn isPrerelease(v: SemVer): bool;
export fn matchesVersionReq(version_str: string, req: string): bool;
export fn isCaretCompatible(ver: string, target: string): bool;
export fn formatVersion(v: SemVer, out: []u8): usize;
export fn bumpMajor(v: SemVer): SemVer;
export fn bumpMinor(v: SemVer): SemVer;
export fn bumpPatch(v: SemVer): SemVer;
