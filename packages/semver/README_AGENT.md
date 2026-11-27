# semver@1.0.0 (Agent Quickref)

> Unified zero-heap SemVer 2.0.0 parser, comparator, npm range evaluator, and Rust Cargo VersionReq matcher for CP (superset of npm semver & crates.io semver) | License: MIT
> Import: `import { ... } from "semver";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseSemver(s: string): SemVer;`
- `export fn compareSemver(a: SemVer, b: SemVer): i32;`
- `export fn parseVersion(s: string): Version;`
- `export fn compareVersions(a: Version, b: Version): i32;`
- `export fn isPrerelease(v: Version): bool;`
- `export fn matchesVersionReq(version_str: string, req: string): bool;`
- `export fn isCaretCompatible(ver: string, target: string): bool;`
- `export fn formatVersion(v: Version, out: []u8): usize;`
- `export fn bumpMajor(v: Version): Version;`
- `export fn bumpMinor(v: Version): Version;`
- `export fn bumpPatch(v: Version): Version;`
- `export fn satisfies(version: string, range: string): bool;`
- `export fn valid(s: string): bool;`
- `export fn compare(a: string, b: string): i32;`
- `export fn gt(a: string, b: string): bool;`
- `export fn gte(a: string, b: string): bool;`
- `export fn lt(a: string, b: string): bool;`
- `export fn lte(a: string, b: string): bool;`
- `export fn eq(a: string, b: string): bool;`
- `export fn neq(a: string, b: string): bool;`
- `export fn major(s: string): u32?;`
- `export fn minor(s: string): u32?;`
- `export fn patch(s: string): u32?;`
- `export fn clean(version: string, out: []u8): usize;`
- *(+7 more in `src/index.d.cp`)*
