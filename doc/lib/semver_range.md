// Source: cp/doc/lib/semver_range.md
# `cap:semver_range` — Semver 2.0 语义化版本与范围表达式选版引擎

纯 CP、零 C 依赖的工业级 Semver 2.0.0 版本解析器、npm/Cargo 风格范围表达式求值器与约束选版求解引擎。

## 导入方式

```cp
import {
    SemverError,
    CompOp,
    RangeOptions,
    Semver,
    Comparator,
    ComparatorSet,
    RangeExpr,
    parseSemver,
    compare,
    comparePrecedence,
    equals,
    lt,
    lte,
    gt,
    gte,
    parseRange,
    satisfies,
    satisfiesWithOptions,
    satisfiesStr,
    satisfiesStrWithOptions,
    maxSatisfying,
    maxSatisfyingWithOptions,
    minSatisfying,
    minSatisfyingWithOptions,
    intersects,
} from "cap:semver_range";

// 或通过动态命名空间挂载：
import cap from "cap";
// cap.semver_range.parseSemver / cap.semver_range.parseRange / cap.semver_range.satisfiesStr
```

## 核心类型与错误建模

### `SemverError`
- `EmptyInput`：输入为空或仅包含空白字符
- `InvalidVersion`：版本号格式非法（缺失段、多余段或非法字符）
- `LeadingZero`：主/次/补丁版本号或纯数字预发布标识符存在非法前导零（如 `01.2.3`、`1.2.3-01`）
- `IntegerOverflow`：版本号数值超过 `u64` 上限
- `InvalidPrerelease`：预发布标签为空或包含非法标识符
- `InvalidBuildMetadata`：构建元数据标签为空或包含非法标识符
- `InvalidRange`：范围表达式整体语法非法（如悬空 `||`）
- `InvalidComparator`：比较器语法非法（如孤立操作符 `^`、`>=` 或未知前缀）
- `InvalidHyphenRange`：连字符区间语法非法（如 `1.2.3 - `）
- `OutOfMemory`：内存分配失败

### `Semver`
- 字段：`major: u64`, `minor: u64`, `patch: u64`, `prerelease: []const u8`, `build: []const u8`
- 方法：`hasPrerelease()`, `hasBuild()`, `format(alloc)`

### `RangeExpr` & `RangeOptions`
- `RangeExpr`：`sets: []ComparatorSet`（`||` 并集，每个 `ComparatorSet` 为空格分隔的交集），通过 `deinit(alloc)` 释放。
- `RangeOptions`：`{ include_prerelease: bool = false }`

## 主要 API

- **版本解析与比较**：
  - `parseSemver(input: []const u8): Semver ! SemverError`（支持前置 `v`/`=` 规范化去除）
  - `compare(a: Semver, b: Semver): i8` / `comparePrecedence(a: Semver, b: Semver): i8`（严格遵循 Semver 2.0 第 11 条规则，忽略 `+build`）
  - `equals(a, b)` / `lt(a, b)` / `lte(a, b)` / `gt(a, b)` / `gte(a, b)`
- **范围解析与求值**：
  - `parseRange(alloc, input: []const u8): RangeExpr ! SemverError`
    - 支持 `=`, `<`, `<=`, `>`, `>=`, `^`（含 `^0.2.3`、`^0.0.3` 零主版本规则）, `~`, `x`/`X`/`*` 通配符, `A - B` 连字符区间, 空格交集, `||` 并集
    - 默认执行 npm/Semver 预发布隔离规则（仅当同 `ComparatorSet` 内存在同 `[major, minor, patch]` 元组的预发布比较器时才匹配预发布版本）
  - `satisfies(version, range): bool` / `satisfiesWithOptions(version, range, opts): bool`
  - `satisfiesStr(alloc, ver_str, range_str): bool ! SemverError` / `satisfiesStrWithOptions(...)`
- **选版与约束求解**：
  - `maxSatisfying(versions, range): Semver?` / `maxSatisfyingWithOptions(...)`
  - `minSatisfying(versions, range): Semver?` / `minSatisfyingWithOptions(...)`
  - `intersects(range_a, range_b): bool`
