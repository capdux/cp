# `cap:coverage_options` — 纯 CP 代码覆盖率配置、阈值判定与 LCOV 生成器（吸收自 Bun `CodeCoverageOptions`）

## 概述

`cap:coverage_options`（亦支持 `cap.coverage_options` 动态挂载）从 Bun `src/options_types/CodeCoverageOptions.cp` 吸收并纯 CP 化：提供覆盖率报告器配置 `Reporters`、最低覆盖率阈值 `CoverageFraction`、指标统计 `CoverageMetrics`、阈值校验 `meetsThresholds` 及标准 LCOV 记录格式化 `formatLcovRecord`。

## 核心类型与 API

- `Reporter` / `Reporters` / `CoverageFraction` / `CoverageMetrics`
- `CodeCoverageOptions`（`meetsThresholds(m)`）
- `formatLcovRecord(alloc, sf_path, lf, lh, fnf, fnh) -> ![]u8`

## 示例

```cp
import { CodeCoverageOptions, CoverageFraction, CoverageMetrics } from "cap:coverage_options";

const opts = CodeCoverageOptions{ .enabled = true, .fractions = CoverageFraction{ .lines = 0.80 } };
const ok = opts.meetsThresholds(CoverageMetrics{ .lines_hit = 9, .lines_total = 10 });
```
