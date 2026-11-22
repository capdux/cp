# `cap:lcov_report` — 零堆分配 LCOV 代码覆盖率解析与序列化库

> 参考来源：/data/dev/bun/src/options_types/CodeCoverageOptions.cp | 吸收日期：2026-11-22

## 概述

`cap:lcov_report` 吸收自 Bun 测试覆盖率子系统配置与报告模型（`src/options_types/CodeCoverageOptions.cp`），提供纯 CP、零外部依赖、零堆分配的 LCOV 覆盖率数据结构、行命中累加、覆盖率百分比与 CI 门禁阈值校验（对应上游 `fail_on_low_coverage` + `fractions` 策略），以及标准 LCOV 记录块解析与序列化能力。

## 上游行为契约与规范细节

- **覆盖率门禁策略（对齐 Bun `CodeCoverageOptions`）**：Bun 测试运行器支持按 `lines` / `functions` / `branches` 三个维度配置最低覆盖率阈值并在低于阈值时失败（`fail_on_low_coverage`）；CP 版通过 `CoverageThresholds` 与 `FileCoverage.meetsThresholds(t)` 直接提供三维度门禁校验。
- **空文件 100% 惯例**：当 `lines_found == 0`、`functions_found == 0` 或 `branches_found == 0` 时，对应 `lineCoveragePct()` / `functionCoveragePct()` / `branchCoveragePct()` 按 LCOV / Istanbul 通用惯例返回 `100.0`，避免 `0 / 0` 除零产生 `NaN`。
- **同号行命中合并**：多次对同一行号调用 `recordLineHit` 会累加 `hits` 计数，并在首次从 `0` 变为 `> 0` 时自动递增 `lines_hit`，天然支持多测试用例覆盖率合并。
- **严格 LCOV 块边界校验**：`parseLcovRecord` 要求记录块同时包含 `SF:` 源文件路径与 `end_of_record` 终结标记；兼容跳过可选的 `TN:`、`FN:`、`FNDA:`、`BRDA:` 扩展行，遇到缺失关键行或畸形字段时安全返回 `null`。

## 核心类型与 API

### `LineHit` & `CoverageThresholds`

```cp
pub struct LineHit {
    line_no: u32 = 0,
    hits: u64 = 0,
}

pub struct CoverageThresholds {
    lines_pct: f32 = 0.0,
    functions_pct: f32 = 0.0,
    branches_pct: f32 = 0.0,
}
```

### `FileCoverage`

```cp
pub struct FileCoverage {
    source_file: []const u8 = "",
    lines: [128]LineHit = [LineHit{}; 128],
    line_count: usize = 0,
    lines_found: u32 = 0,
    lines_hit: u32 = 0,
    functions_found: u32 = 0,
    functions_hit: u32 = 0,
    branches_found: u32 = 0,
    branches_hit: u32 = 0,

    static fn init(source_file: []const u8): FileCoverage
    fn recordLineHit(*_, line_no: u32, hits: u64): bool
    fn lineCoveragePct(): f32
    fn functionCoveragePct(): f32
    fn branchCoveragePct(): f32
    fn meetsThresholds(t: CoverageThresholds): bool
    fn formatLcov(out: []u8): ([]const u8)?
}
```

- `recordLineHit`：支持同号行累加命中次数并自动维护 `lines_found` / `lines_hit`；拒绝 `line_no == 0` 或超出 128 行槽位容量（返回 `false`）。
- `formatLcov`：输出标准 `SF:<path>\n[FNF/FNH]\n[BRF/BRH]\nDA:<line>,<hits>\n...\nLF:<found>\nLH:<hit>\nend_of_record\n`，输出缓冲不足返回 `null`。

### `parseLcovRecord`

```cp
pub fn parseLcovRecord(input: []const u8): FileCoverage?
```

## 边界注意事项

1. **行号 1-based 约束**：LCOV 行号从 `1` 开始，`recordLineHit(0, ...)` 返回 `false`。
2. **零堆分配容量上限**：单个 `FileCoverage` 栈内最多追踪 128 条不重复行记录，超出时 `recordLineHit` 返回 `false` 而不会溢出。

## 使用示例

```cp
import { FileCoverage, CoverageThresholds, parseLcovRecord } from "cap:lcov_report";

let cov = FileCoverage.init("src/app.cp");
* = cov.recordLineHit(10, 3);
* = cov.recordLineHit(11, 0);
* = cov.recordLineHit(11, 2); // 累加至 2 次命中，lines_hit 自动更新

const passed = cov.meetsThresholds(CoverageThresholds{ lines_pct: 80.0 });
let buf: [512]u8 = [0; 512];
const lcov_text = cov.formatLcov(buf[0..]).?;
const parsed = parseLcovRecord(lcov_text).?;
* = passed;
* = parsed;
```
