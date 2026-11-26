# @cap/semver-range

零堆分配的 CP 原生 SemVer 2.0.0 与 npm 版本范围求值引擎，Clean-Room MIT 实现对标 npm 全站 Top 1 顶流包 `semver`（4 亿+/周下载）。

## 特性

- **`parseSemver(s)`**：支持可选 `v`/`=` 前缀、预发布标识 `-alpha.1`、构建元数据 `+build.123`，严格拒绝前导零（如 `01.2.3`）与不完整版本
- **`compareSemver(a, b)`**：严格遵循 SemVer 2.0.0 §11 排序规则（数字段 < 字母段、无预发布 > 有预发布、忽略构建元数据）
- **`satisfies(version, range)`**：完整支持 npm `node-semver` 范围语法：
  - 精确与比较：`1.2.3` / `=1.2.3` / `>` / `>=` / `<` / `<=`
  - Caret 插入符：`^1.2.3` / `^0.2.3` / `^0.0.3`
  - Tilde 波浪符：`~1.2.3` / `~1.2`
  - X-Range 通配符：`*` / `x` / `1.x` / `1.2.x`
  - 连字符区间：`1.2.3 - 2.3.4`
  - 逻辑组合：空格交集 `>=1.0.0 <2.0.0` 与 `||` 并集 `^1.0.0 || ^2.0.0`
- **`maxSatisfying` / `minSatisfying`**：零堆返回版本数组中命中的最大/最小版本索引
- **`incMajor` / `incMinor` / `incPatch` & `formatSemver`**

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`npm/node-semver`](https://github.com/npm/node-semver)。
