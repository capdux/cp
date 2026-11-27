# semver-diff@1.0.0 (Agent Quickref)

> Zero-allocation SemVer upgrade diff classifier (major, minor, patch, pre*, build) and breaking-upgrade detector for CP, reusing @cap/semver-range. | License: MIT
> Import: `import { ... } from "semver-diff";` | Full types: `src/index.d.cp`
> Deps: `semver`

## Functions
- `export fn svdfBuildEq(a: SemVer, b: SemVer): bool;`
- `export fn semverDiffKind(ver_a: string, ver_b: string): u8;`
- `export fn semverDiff(ver_a: string, ver_b: string): string?;`
- `export fn isBreakingUpgrade(ver_a: string, ver_b: string): bool;`
