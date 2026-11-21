# `cap:lcov_report` — 零堆分配 LCOV 代码覆盖率解析与序列化库

> 来源：吸收自 `/data/dev/bun/src/options_types/CodeCoverageOptions.cp`

`cap:lcov_report` 提供纯 CP、零外部依赖、零堆分配的 LCOV 覆盖率报告模型、行命中累加、覆盖率百分比与门禁阈值校验，以及标准 LCOV 记录块解析与序列化能力。

## 核心类型

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

- `recordLineHit`：支持同号行累加命中次数并自动维护 `lines_found` / `lines_hit`；拒绝 `line_no == 0` 或超出 128 行容量。
- `lineCoveragePct` / `functionCoveragePct` / `branchCoveragePct`：当对应 `*_found == 0` 时按 LCOV 惯例返回 `100.0`。
- `formatLcov`：输出标准 `SF:<path>\n[FNF/FNH]\n[BRF/BRH]\nDA:<line>,<hits>\n...\nLF:<found>\nLH:<hit>\nend_of_record\n`。

### `parseLcovRecord`

```cp
pub fn parseLcovRecord(input: []const u8): FileCoverage?
```

- 解析单个 LCOV 记录块中的 `SF:`、`DA:`、`FNF:`/`FNH:`、`BRF:`/`BRH:`、`LF:`/`LH:` 与 `end_of_record`（自动跳过可选 `TN:`/`FN:`/`FNDA:`/`BRDA:`）。
- 缺失 `SF:`、缺失 `end_of_record` 或存在畸形行时返回 `null`。

## 使用示例

```cp
import { FileCoverage, CoverageThresholds, parseLcovRecord } from "cap:lcov_report";

let cov = FileCoverage.init("src/app.cp");
* = cov.recordLineHit(10, 3);
* = cov.recordLineHit(11, 0);
* = cov.recordLineHit(11, 2); // 累加至 2 次命中，lines_hit 自动更新

let buf: [512]u8 = [0; 512];
const lcov_text = cov.formatLcov(buf[0..]).?;
const parsed = parseLcovRecord(lcov_text).?;
```
