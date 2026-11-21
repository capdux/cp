# `cap:pkg_matcher`

纯 CP 零堆分配 PNPM 兼容包名通配符匹配器，吸收自 `/data/dev/bun/src/install/PnpmMatcher.cp`。

## 导出类型与函数

- `MatcherBehavior`：`all_include | all_exclude | mixed`
- `PackagePattern`：单条包含/排除规则
- `PackageMatcher`：最多 32 条规则的包名匹配器（`init` / `fromPatterns` / `addPattern` / `isMatch`），严格实现 `@pnpm/matcher` 三态语义（全包含 / 全排除 / 混合排除优先）
- `wildcardMatch(pattern, text)`：线性时间无回溯爆炸的 `*` / `?` 通配符匹配
