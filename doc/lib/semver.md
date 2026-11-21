# `cap:semver` — Semver 2.0.0 语义化版本解析与区间查询引擎

> **上游溯源**: `/data/dev/bun/src/semver/` (`semver.cp`, `Version.cp`, `SemverQuery.cp`, `SemverRange.cp`)  
> **模块路径**: `lib/semver/`  
> **导入协议**: `import { Semver, Version, PartialVersion, ReleaseType, SemverRange, Query } from "cap:semver";`

---

## 一、 架构概述与设计亮点

`cap:semver` 吸收自 Bun 包管理器核心的语义化版本引擎（`bun/src/semver`），严格遵循 **Semantic Versioning 2.0.0** 官方规范，同时兼容 npm / Bun 生态的全部高级 Range 表达式与排序过滤 API：

1. **双模版本解析（严格 Semver 2.0.0 vs 宽松兼容解析）**：
   - `Version.parseStrict`：严格执行 Semver 2.0.0 规范，要求完整 `major.minor.patch`，禁止前导 `v`/`=`、禁止数值段前导零（如 `01.2.3` 或 `1.0.0-01`）、禁止空 `prerelease`/`build` 标签及非法字符。
   - `Version.parse` / `Version.parseLoose`：兼容前导 `v`/`V`/`=`、部分版本（`1.2`、`1`）以及无连字符预发布标签（如 `1.0.0rc.1`），直接将结果零拷贝映射为栈上紧凑结构体 `Version`。
   - `Semver.valid` / `Semver.clean`：校验并输出规范化格式字符串。
2. **Semver 2.0.0 第 11 节优先级阶梯算法 (`Version.order` / `compare`)**：
   - `major` -> `minor` -> `patch` 依次数值比对；正式版优先级高于预发布版（`1.0.0 > 1.0.0-rc.1`）。
   - 预发布号按 `.` 切分标识符逐段对比：纯数字标识符支持任意长度无溢出数值比较（`beta.2 < beta.11`），纯数字标识符优先级始终低于非数字标识符，非数字标识符按 ASCII 字典序比对；构建元数据（`+build`）在优先级比对中忽略。
3. **完备合取/析取 Range 引擎与预发布隔离门控 (`Query` & `SemverRange`)**：
   - 全面支持 `^`、`~`、`~>`、`>=`、`<=`、`>`、`=<`、`=`、`==`、`||` 并集、空格交集、连字符区间 `A - B`（含 `1.0.0 - *` 与部分版本）、通配符 `1.x`/`1.2.*`/`*` 以及部分比较边界（`<=1.2.x`、`>1.x`）。
   - 严格实现 npm / Bun 预发布匹配隔离规则：预发布版本默认仅当同一 ComparatorSet 中存在共享相同 `[major, minor, patch]` 元组的预发布比较器时才允许匹配；亦可通过 `satisfiesWithOptions(..., true)` 开启 `include_prerelease`。
4. **排序、最值筛选与 Range 合法性校验**：
   - 内置 `Semver.sort` / `rsort` / `sortStr` / `rsortStr`、`maxSatisfying` / `minSatisfying` 以及 `validRange` / `isValidRange` / `validRangeBuf`。

---

## 二、 Range 表达式语法规则表

| 语法类型 | 表达式示例 | 展开后的等价比较算子集合 | 语义说明 |
| :--- | :--- | :--- | :--- |
| **Caret 兼容区间 (`^`)** | `^1.2.3` | `>=1.2.3 <2.0.0` | 允许不改变最左侧非零版本号的更新 |
| **Caret 零主版本 (`^0.y.z`)** | `^0.2.3` | `>=0.2.3 <0.3.0` | `major == 0` 时锁定 `minor` 版本 |
| **Caret 双零版本 (`^0.0.z`)** | `^0.0.3` | `>=0.0.3 <0.0.4` | `major == 0, minor == 0` 时仅匹配精确 `patch` |
| **Tilde 补丁区间 (`~` / `~>`)** | `~1.2.3` / `~> 1.2` | `>=1.2.3 <1.3.0` / `>=1.2.0 <1.3.0` | 指定了次版本号时只允许修订号升级 |
| **X-Range 通配区间** | `1.x` / `1.2.*` / `*` | `>=1.0.0 <2.0.0` / `>=1.2.0 <1.3.0` | `*`, `x`, `X` 均可作为通配占位符 |
| **部分比较边界** | `<=1.2.x` / `>1.x` | `<1.3.0` / `>=2.0.0` | 自动将部分版本展开为对应上下界 |
| **Hyphen 连字符区间** | `1.2.3 - 2.3.4` / `1.0.0 - *` | `>=1.2.3 <=2.3.4` / `>=1.0.0` | 支持完整/部分右边界及 `*` 无上界 |
| **空格合取交集 (AND)** | `>= 1.2.0 < 2.0.0` | `>=1.2.0` 且 `<2.0.0` | 算子后可带空格，同时支持 `==` |
| **双竖线析取并集 (OR)** | `^1.0.0 \|\| ^3.0.0` | 满足左子句 **或** 满足右子句 | 多子句之间以 `\|\|` 分隔 |

---

## 三、 `cap:semver` 与 `Bun.semver` / `node-semver` API 对照表

| 功能描述 | `cap:semver` API | `Bun.semver` / `node-semver` 对照 | 返回类型 |
| :--- | :--- | :--- | :--- |
| **解析版本字符串** | `Semver.parse` / `parseStrict` / `parseLoose` | `semver.parse(str, { loose })` | `Version!` |
| **版本规范化校验** | `Semver.valid(str, buf)` / `Semver.clean(str, buf)` | `semver.valid(str)` / `semver.clean(str)` | `([]const u8)?` |
| **版本三路比较** | `Semver.order(a, b)` / `Semver.compare(a, b)` | `Bun.semver.order(a, b)` | `std.math.Order` |
| **版本数组排序** | `Semver.sort` / `rsort` / `sortStr` / `rsortStr` | `semver.sort(list)` / `semver.rsort(list)` | 原地排序 |
| **区间校验（结构体/字符串）** | `Semver.satisfies` / `satisfiesStr` / `*WithOptions` | `Bun.semver.satisfies(v, range)` | `bool` |
| **Range 合法性校验** | `Semver.validRange(range)` / `isValidRange` / `validRangeBuf` | `semver.validRange(range)` | `([]const u8)?` / `bool` |
| **查找最大/最小满足版本** | `Semver.maxSatisfying` / `minSatisfying` / `*Str` | `semver.maxSatisfying` / `minSatisfying` | `Version?` / `([]const u8)?` |
| **版本号递增** | `v.increment(release_type, identifier?)` | `semver.inc(v, release, identifier)` | `Version` |

---

## 四、 代码示例

### 1. 严格/宽松解析、预发布比较与版本排序

```typescript
const std = @import("std");
import { Semver, Version } from "cap:semver";

fn demoVersionOrderAndSort(): ! {
    let v1 = try Semver.parseStrict("1.0.0-alpha.2");
    let v2 = try Semver.parseStrict("1.0.0-alpha.11");
    let v3 = try Semver.parseLoose("=v1.0.0");

    // 数值段比较：2 < 11，且预发布版 < 正式版
    let c1 = Semver.order(v1, v2) == .lt; // true
    let c2 = v2.lt(v3);                   // true

    // 字符串版本数组原地升序排列
    let list: [4][]const u8 = [ "1.10.0", "1.2.0", "1.0.0-rc.1", "1.0.0" ];
    Semver.sortStr(&list);
    // list => ["1.0.0-rc.1", "1.0.0", "1.2.0", "1.10.0"]
}
```

### 2. Range 合法性校验、预发布门控与候选筛选

```typescript
import { Semver } from "cap:semver";

fn demoRangeMatching() {
    // 1. 预发布默认隔离 vs 显式匹配
    let r1 = Semver.satisfiesStr("2.0.0-alpha.1", "^1.0.0");                 // false
    let r2 = Semver.satisfiesStr("1.2.3-beta.4", "^1.2.3-beta.2");           // true
    let r3 = Semver.satisfiesStrWithOptions("2.0.0-alpha.1", "^1.0.0", true); // true

    // 2. Range 合法性检验与规范化输出
    let ok_range = Semver.validRange("^1.2.3 || >=2.0.0"); // non-null
    let bad_range = Semver.validRange("^1.0.0 || invalid"); // null

    let canon_buf: [128]u8 = undefined;
    let canon = Semver.validRangeBuf("^1.2.3", &canon_buf); // ">=1.2.3 <2.0.0"

    // 3. 最值筛选
    let candidates: [4][]const u8 = [ "1.0.0", "1.4.2", "1.9.0", "2.0.0" ];
    let best = Semver.maxSatisfyingStr(candidates[0..4], "^1.0.0"); // "1.9.0"
}
```

---

## 五、 扩展子模块索引（零堆分配纯 CP 组件）

`cap:semver` 已将以下 5 个零堆分配子模块统一收敛至 `lib/semver/` 并通过 `index.cp` 聚合导出：

| 子文件 | 核心导出符号 | 核心能力 |
| :--- | :--- | :--- |
| `lib/semver/bump.cp` | `BumpKind`, `bumpMajor`, `bumpMinor`, `bumpPatch`, `stripPrereleaseAndBuild`, `bumpPrerelease`, `bumpVersion` | SemVer 2.0.0 版本号递增、预发布标识符晋升与构建元数据剥离 |
| `lib/semver/spec.cp` | `DependencyKind`, `WorkspaceSpecKind`, `WorkspaceSpec`, `GitSpec`, `NpmAliasSpec`, `DependencySpec`, `PackageName` | npm/workspace/git/file/alias 依赖说明符解析与规范化 |
| `lib/semver/rset.cp` | `MAX_COMPARATORS_PER_GROUP`, `MAX_GROUPS`, `SemverVersion`, `RangeOp`, `RangeComparator`, `ComparatorGroup`, `ParsedSemverRange` | 固定容量零堆分配 Range 比较器组解析与求值 |
| `lib/semver/satisfies.cp` | `SatisfiesVersion`, `satisfiesRange`, `satisfiesVersion`, `isValidSemverRange` | 轻量零分配版本区间 `satisfies` 校验器 |
| `lib/semver/sort.cp` | `SemverOrder`, `ParsedSemver`, `parseSemverStrict`, `parseSemverLoose`, `compareSemver`, `compareSemverStr`, `sortSemverAsc`, `sortSemverDesc`, `maxSemverIndex`, `minSemverIndex` | SemVer 2.0.0 严格/宽松解析、原地升降序排序与极值索引查询 |
