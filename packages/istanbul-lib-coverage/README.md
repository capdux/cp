[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# istanbul-lib-coverage

Zero-heap code coverage metrics, summary aggregation, threshold checking, and LCOV codec in pure CP.

## API

- `CoverageMetric { total: u32, covered: u32, skipped: u32 }`
  - `pct(): f64`
  - `merge(other: CoverageMetric): CoverageMetric`
- `CoverageSummary { lines: CoverageMetric, statements: CoverageMetric, functions: CoverageMetric, branches: CoverageMetric }`
  - `merge(other: CoverageSummary): CoverageSummary`
  - `isAboveThreshold(min_pct: f64): bool`
- `LineHit { line: u32, hits: u32 }`
- `formatLcovRecord(file_path: string, hits: []const LineHit, out: []u8): usize`
- `parseLcovSummary(lcov_text: string): CoverageMetric`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`istanbul-lib-coverage@3.2.2`)
- **Upstream Repository**: https://github.com/istanbuljs/istanbuljs
- **License**: BSD-3-Clause (clean-room CP implementation)
