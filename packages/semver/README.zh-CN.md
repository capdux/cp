[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `semver`

面向 CP 语言的零堆分配 SemVer 2.0.0 解析器、比较器、npm 范围求值器与 Rust Cargo `VersionReq` 匹配器——同时完整兼容 **npm `semver`** 与 **crates.io `semver`** 双生态超集。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `npm:semver` ([npm/node-semver](https://github.com/npm/node-semver), v7 对齐) & `crates.io:semver` ([dtolnay/semver](https://github.com/dtolnay/semver), v1 对齐) |
| **CP 包名与版本** | `semver@1.0.0` (`MIT`) |
| **外部依赖** | 无（仅依赖内置 `cap:ascii`、`cap:fmt`） |

保持原包名导入，零改名成本直接迁移：

```ts
// TypeScript (npm semver)
import semver from "semver";
semver.satisfies("1.5.0", "^1.2.0 || ~2.0.0");
semver.gt("2.0.0", "1.9.9");
```

```rust
// Rust (crates.io semver)
use semver::{Version, VersionReq};
let v = Version::parse("1.4.2")?;
let req = VersionReq::parse(">=1.2.0, <2.0.0")?;
req.matches(&v);
```

```cp
// CP — 保持原包名，无缝融合双生态 API
import { Version, VersionReq, satisfies, gt } from "semver";

const v = Version.parse("1.4.2");
const req = VersionReq.parse(">=1.2.0, <2.0.0");
const ok = req.matches(v);                              // true
const in_range = satisfies("1.5.0", "^1.2.0 || ~2.0.0"); // true
const higher = gt("2.0.0", "1.9.9");                     // true
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 核心优化架构 |
| :--- | :--- | --: | --: | :--- |
| `parseSemver` + `compareSemver` | 2× 含 prerelease + build 完整版本 | **156.40 ns/op** | **6,393,994 ops/s** | 16-lane SIMD 数字/标识符扫描、零堆分配 |

- **零堆分配**：`SemVer` / `Version` 在栈上内联存储 `pre: [32]u8` 与 `build: [32]u8`，零动态内存申请。
- **16-lane SIMD 加速**：批量校验版本号数字与标识符段，结合 `undefined` 缓冲避免无效栈清零。

## 实战示例

### 1. 基础解析、版本比较与 npm 范围匹配

```cp
import { parseSemver, compareSemver, satisfies, valid, gt, major, minor, patch } from "semver";

fn demoBasic() {
    const a = parseSemver("1.2.3-alpha.1+build.42");
    const b = parseSemver("1.2.3-alpha.2");
    if (a.valid && b.valid) {
        const ord = compareSemver(a, b); // -1 (a < b)
        const is_pre = a.isPrerelease(); // true
        _ = ord;
        _ = is_pre;
    }

    const ok = satisfies("1.5.0", "^1.2.0 || ~2.0.0"); // true
    const is_valid = valid("1.0.0-rc.1");              // true
    const newer = gt("2.0.0", "1.9.9");                // true
    const maj = major("3.4.5") orelse 0;               // 3
    const min = minor("3.4.5") orelse 0;               // 4
    const pat = patch("3.4.5") orelse 0;               // 5
    _ = ok; _ = is_valid; _ = newer; _ = maj; _ = min; _ = pat;
}
```

### 2. Rust Cargo `VersionReq`、版本递增与格式化

```cp
import { Version, VersionReq, bumpMinor, formatVersion, matchesVersionReq, isCaretCompatible } from "semver";

fn demoCargoStyle() {
    const req = VersionReq.parse(">=1.2.0, <2.0.0");
    if (req.valid) {
        const matched = req.matchesStr("1.8.3"); // true
        _ = matched;
    }

    const v = Version.parse("1.4.2");
    const next = bumpMinor(v); // 1.5.0
    let out: [64]u8 = undefined;
    const n = formatVersion(next, out[0..]); // out[0..n] == "1.5.0"

    const quick = matchesVersionReq("1.4.2", "^1.2");   // true
    const compat = isCaretCompatible("1.4.2", "1.2.0"); // true
    _ = n; _ = quick; _ = compat;
}
```

### 3. 进阶边界场景：`clean`、`inc`、`maxSatisfying` 与非法输入校验

```cp
import { parseSemver, clean, inc, maxSatisfying, minSatisfying } from "semver";

fn demoEdgeCases() {
    // 非法版本返回 valid == false，零异常零分配
    const bad = parseSemver("not-a-version");
    if (!bad.valid) {
        // 安全处理非法输入
    }

    // 调用方栈缓冲写入：输入非法或缓冲不足时返回 0
    let buf: [64]u8 = undefined;
    const clean_len = clean("  =v1.2.3  ", buf[0..]); // "1.2.3"
    const inc_len = inc("1.2.3", "minor", buf[0..]);  // "1.3.0"

    // 从候选数组中查找最大/最小满足范围的下标（无匹配返回 -1）
    const candidates = [_]string{ "1.0.0", "1.4.2", "1.9.0", "2.0.0" };
    const max_idx = maxSatisfying(candidates[0..], "^1.0.0"); // 2 ("1.9.0")
    const min_idx = minSatisfying(candidates[0..], "^1.0.0"); // 0 ("1.0.0")
    _ = clean_len; _ = inc_len; _ = max_idx; _ = min_idx;
}
```

## 核心 API 速览

- **核心类型**：`SemVer`（别名 `Version`）、`VersionReq`
- **npm 风格 API**：`parseSemver`、`compareSemver`、`satisfies`、`valid`、`compare`、`gt`/`gte`/`lt`/`lte`/`eq`/`neq`、`major`/`minor`/`patch`、`clean`、`inc`、`maxSatisfying`、`minSatisfying`、`incMajor`/`incMinor`/`incPatch`、`formatSemver`
- **Cargo 风格 API**：`Version.parse`、`VersionReq.parse`、`parseVersion`、`compareVersions`、`isPrerelease`、`matchesVersionReq`、`isCaretCompatible`、`formatVersion`、`bumpMajor`/`bumpMinor`/`bumpPatch`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
