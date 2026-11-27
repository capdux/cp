[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# semver

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Unified zero-heap SemVer 2.0.0 parser, comparator, npm range evaluator, and Rust Cargo VersionReq matcher for CP (superset of npm semver & crates.io semver)

## 安装与导入

```bash
cap add semver
```

```cp
import { parseSemver } from "semver";
```

## 核心导出 API

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
- `export fn inc(version: string, release: string, out: []u8): usize;`
- `export fn maxSatisfying(versions: &[]string, range: string): i32;`
- `export fn minSatisfying(versions: &[]string, range: string): i32;`
- `export fn incMajor(v: SemVer): SemVer;`
- `export fn incMinor(v: SemVer): SemVer;`
- `export fn incPatch(v: SemVer): SemVer;`
- `export fn formatSemver(v: SemVer, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`semver@1.0.0`)，许可证：ISC
- **上游生态**：`crates.io` (`semver@1.0.0`)，许可证：MIT
