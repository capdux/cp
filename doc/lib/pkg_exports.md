# `cap:pkg_exports` — 纯 CP Node.js / Bun `package.json` `"exports"` 条件子路径解析引擎

## 概述

`cap:pkg_exports` 吸收自 Bun `src/resolver/package_json.cp`，纯 CP 零外部依赖、零堆分配，严格遵循 Node.js `PACKAGE_EXPORTS_RESOLVE` 与 `PATTERN_KEY_COMPARE` 规范实现精确子路径、通配符最长前缀/后缀竞争、`null` 私有子路径阻断、条件导出顺序求值及 `..` 路径穿越拦截。

## 核心类型与 API

- `ExportTargetKind`：`path` / `blocked`
- `ExportConditionRule`：`toPath(condition, target)` / `block(condition)`
- `ExportSubpathEntry`：`subpath` + 最多 8 条条件规则
- `ResolveStatus`：`matched` / `blocked` / `not_found` / `invalid_subpath` / `buffer_too_small`
- `ResolveResult`：`status`、`path`、`matched_condition`（`isMatched()` / `isBlocked()` / `isNotFound()`）
- `ExportsMap`（固定容量 32 条目）：`init()`、`addSubpath(subpath, rules)`、`addDirect(subpath, target)`、`blockSubpath(subpath)`、`resolve(subpath, active_conditions, out)`

## 示例

```cp
import { ExportsMap, ExportConditionRule } from "cap:pkg_exports";

let map = ExportsMap.init();
const root_rules: [3]ExportConditionRule = [
    ExportConditionRule.toPath("bun", "./src/index.ts"),
    ExportConditionRule.toPath("import", "./dist/index.mjs"),
    ExportConditionRule.toPath("default", "./dist/index.cjs"),
];
* = map.addSubpath(".", root_rules[0..]);
let out: [128]u8 = std.mem.zeroes([128]u8);
const conds: [2][]const u8 = ["bun", "import"];
const res = map.resolve(".", conds[0..], out[0..]);
// res.path == "./src/index.ts"
```
