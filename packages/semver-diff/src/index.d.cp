// Type definitions for CP

import { SemVer, parseSemver, compareSemver } from "../../semver-range/src/index.cp";
export fn svdfBuildEq(a: SemVer, b: SemVer): bool;
export fn semverDiffKind(ver_a: string, ver_b: string): u8;
export fn semverDiff(ver_a: string, ver_b: string): string?;
export fn isBreakingUpgrade(ver_a: string, ver_b: string): bool;
