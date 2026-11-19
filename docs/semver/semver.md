# `cap:semver` — Semver 2.0.0 语义化版本解析与区间查询引擎

> **上游溯源**: `/data/dev/bun/src/semver/` (`semver.cp`, `Version.cp`, `SemverQuery.cp`, `SemverRange.cp`)  
> **模块路径**: `lib/semver/`  
> **导入协议**: `import { Semver, Version, PartialVersion, ReleaseType, SemverRange, Query } from "cap:semver";`

---

## 一、 架构概述与设计亮点

`cap:semver` 吸收自 Bun 包管理器核心的语义化版本引擎（`bun/src/semver`），严格遵循 **Semantic Versioning 2.0.0** 官方规范，同时兼容 npm / cap 生态的全部高级 Range 表达式语法：

1. **零堆分配版本解析 (`Version.parse`)**：直接将 `"v1.2.3-alpha.1+build.42"` 解析为栈上紧凑结构体 `Version { major, minor, patch, prerelease, build }`，其中 `prerelease` 与 `build` 直接借用原输入切片的零拷贝视图。
2. **Semver 2.0.0 第 11 节优先级阶梯算法 (`Version.order`)**：
   - 主版本号 (`major`) -> 次版本号 (`minor`) -> 修订号 (`patch`) 依次数值比对。
   - 正式版优先级高于预发布版（`1.0.0 > 1.0.0-rc.1`）。
   - 预发布号按 `.` 切分标识符逐段对比：纯数字标识符按数值大小比对（`beta.2 < beta.11`），非数字标识符按 ASCII 字典序比对，且纯数字标识符优先级始终低于非数字标识符。
   - 构建元数据（`+build`）在优先级比较中自动忽略。
3. **完备的合取/析取 Range 查询引擎 (`Query` & `SemverRange`)**：支持 `^`（Caret）、`~`（Tilde）、`>=` / `<=` / `>` / `<` / `=`（比较算子，兼容算子后带空格或无空格）、`1.x` / `1.2.*` / `*`（通配符）、`1.2.0 - 2.3.0`（连字符区间）以及 `||`（逻辑或析取范式）。

---

## 二、 Range 表达式语法规则表

| 语法类型 | 表达式示例 | 展开后的等价比较算子集合 | 语义说明 |
| :--- | :--- | :--- | :--- |
| **Caret 兼容区间 (`^`)** | `^1.2.3` | `>=1.2.3 <2.0.0` | 允许不改变最左侧非零版本号的更新 |
| **Caret 零主版本 (`^0.y.z`)** | `^0.2.3` | `>=0.2.3 <0.3.0` | `major == 0` 时锁定 `minor` 版本 |
| **Caret 双零版本 (`^0.0.z`)** | `^0.0.3` | `>=0.0.3 <0.0.4` | `major == 0, minor == 0` 时仅匹配精确 `patch` |
| **Tilde 补丁区间 (`~`)** | `~1.2.3` / `~1.2` | `>=1.2.3 <1.3.0` / `>=1.2.0 <1.3.0` | 指定了次版本号时只允许修订号（patch）升级 |
| **X-Range 通配区间** | `1.x` / `1.2.*` / `*` | `>=1.0.0 <2.0.0` / `>=1.2.0 <1.3.0` | `*`, `x`, `X` 均可作为通配占位符 |
| **Hyphen 连字符区间** | `1.2.3 - 2.3.4` | `>=1.2.3 <=2.3.4` | 闭区间匹配；若右端为部分版本 `2.3` 则转为 `<2.4.0` |
| **空格合取交集 (AND)** | `>=1.2.0 <2.0.0` | `>=1.2.0` 且 `<2.0.0` | 支持 `>=1.2.0` 与 `>= 1.2.0` 两种书写风格 |
| **双竖线析取并集 (OR)** | `^1.0.0 \|\| ^3.0.0` | 满足左子句 **或** 满足右子句 | 多子句之间以 `\|\|` 分隔 |

---

## 三、 `cap:semver` 与 `Bun.semver` API 对照表

| 功能描述 | `cap:semver` API | `Bun.semver` / `node-semver` 对照 | 返回类型 |
| :--- | :--- | :--- | :--- |
| **解析版本字符串** | `Semver.parse(str)` / `Version.parse(str)` | `semver.parse(str)` | `!Version` |
| **版本三路比较** | `Semver.order(a, b)` / `Version.order(a, b)` | `Bun.semver.order(a, b)` | `std.math.Order` (`.lt`, `.eq`, `.gt`) |
| **关系比较方法** | `v.eql(b)`, `v.lt(b)`, `v.lte(b)`, `v.gt(b)`, `v.gte(b)` | `semver.eq`, `lt`, `lte`, `gt`, `gte` | `bool` |
| **区间校验（结构体）** | `Semver.satisfies(ver, range)` | `Bun.semver.satisfies(v, range)` | `bool` |
| **区间校验（字符串）** | `Semver.satisfiesStr(ver_str, range)` | `Bun.semver.satisfies(ver_str, range)` | `bool` |
| **查找最大满足版本** | `Semver.maxSatisfying(versions, range)` | `semver.maxSatisfying(versions, range)` | `Version?` |
| **查找最小满足版本** | `Semver.minSatisfying(versions, range)` | `semver.minSatisfying(versions, range)` | `Version?` |
| **字符串列表最大/最小满足** | `Semver.maxSatisfyingStr` / `minSatisfyingStr` | `semver.maxSatisfying(strs, range)` | `([]const u8)?` |
| **版本号递增** | `v.increment(release_type, identifier?)` | `semver.inc(v, release, identifier)` | `Version` |

---

## 四、 代码示例

### 1. 版本解析、格式化与 Semver 2.0.0 优先级比较

```typescript
const std = @import("std");
import { Semver, Version } from "cap:semver";

fn demoVersionOrder(): !void {
    let v1 = try Semver.parse("1.0.0-alpha.2");
    let v2 = try Semver.parse("1.0.0-alpha.11");
    let v3 = try Semver.parse("1.0.0");

    // 数值段比较：2 < 11，且预发布版 < 正式版
    let cond1 = v1.lt(v2); // true
    let cond2 = v2.lt(v3); // true

    // 版本号递增
    let v_next = v3.increment(.preminor, "beta");
    let buf: [64]u8 = undefined;
    let formatted = v_next.format(&buf);
    // formatted => "1.1.0-beta"
}
```

### 2. Range 表达式匹配与候选版本筛选

```typescript
import { Semver } from "cap:semver";

fn demoRangeMatching(): void {
    // 1. Caret / Tilde / 组合区间校验
    let ok1 = Semver.satisfiesStr("1.8.4", "^1.2.0");                // true
    let ok2 = Semver.satisfiesStr("1.2.9", "~1.2.3");                // true
    let ok3 = Semver.satisfiesStr("2.5.0", ">= 1.5.0 < 3.0.0");      // true
    let ok4 = Semver.satisfiesStr("3.1.0", "^1.0.0 || >=3.0.0");     // true

    // 2. 从候选版本列表中选出满足区间的最高/最低版本
    let list: [5][]const u8 = [ "1.0.0", "1.2.0", "1.5.4", "2.0.0", "2.1.0" ];
    let best = Semver.maxSatisfyingStr(list[0..5], "^1.0.0");
    // best.? => "1.5.4"

    let lowest = Semver.minSatisfyingStr(list[0..5], ">=1.2.0 <2.0.0");
    // lowest.? => "1.2.0"
}
```
