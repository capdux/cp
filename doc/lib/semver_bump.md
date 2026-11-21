# `cap:semver_bump` — SemVer 2.0.0 版本号递增与格式化库

> 来源：吸收自 `/data/dev/bun/src/install/semver.zig`（SemVer 2.0.0 Version Bumper & Formatter）

`cap:semver_bump` 提供纯 CP、零外部依赖、零堆分配的 SemVer 2.0.0 版本号递增与规范化输出能力，遵循 npm `node-semver` 晋升与预发布递增语义。

## 核心类型与函数

### `BumpKind`

```cp
pub enum BumpKind: u8 {
    major = 0,
    minor = 1,
    patch = 2,
    prerelease = 3,
    release = 4,

    fn asStr(): string
}
```

### 核心函数

```cp
pub fn bumpMajor(version: []const u8, out: []u8): ([]const u8)?
pub fn bumpMinor(version: []const u8, out: []u8): ([]const u8)?
pub fn bumpPatch(version: []const u8, out: []u8): ([]const u8)?
pub fn bumpPrerelease(version: []const u8, preid: []const u8, out: []u8): ([]const u8)?
pub fn stripPrereleaseAndBuild(version: []const u8, out: []u8): ([]const u8)?
pub fn bumpVersion(version: []const u8, kind: BumpKind, preid: []const u8, out: []u8): ([]const u8)?
```

- `bumpMajor`：`1.2.3` → `2.0.0`、`2.0.0-alpha.1` → `2.0.0`（直接晋升）、`1.2.3-beta.0` → `2.0.0`。
- `bumpMinor`：`1.2.3` → `1.3.0`、`1.3.0-alpha.1` → `1.3.0`（直接晋升）、`1.2.3-alpha.1` → `1.3.0`。
- `bumpPatch`：`1.2.3` → `1.2.4`、`1.2.4-alpha.1` → `1.2.4`（直接晋升）。
- `bumpPrerelease`：`1.2.3` + `"alpha"` → `1.2.4-alpha.0`、`1.2.4-alpha.0` + `"alpha"` → `1.2.4-alpha.1`、`1.2.4-alpha.1` + `"beta"` → `1.2.4-beta.0`。
- `stripPrereleaseAndBuild`：去除 `-prerelease` 与 `+build` 元数据。
- 支持可选前导 `v` 前缀；严格校验 SemVer 2.0.0 语法（拒绝前导零、非法字符与缓冲区溢出）。

## 使用示例

```cp
import { BumpKind, bumpVersion, bumpPrerelease } from "cap:semver_bump";

let buf: [64]u8 = [0; 64];
const v1 = bumpPrerelease("1.2.3", "alpha", buf[0..]).?;       // "1.2.4-alpha.0"
const v2 = bumpVersion("2.0.0-rc.1", .major, "", buf[0..]).?;  // "2.0.0"
```
