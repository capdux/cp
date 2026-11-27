[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hdrhistogram

> 版本：`1.0.0` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 hdrhistogram

## 安装与导入

```bash
cap add hdrhistogram
```

```cp
import { hdr_init } from "hdrhistogram";
```

## 核心导出 API

- `export fn hdr_init(lowest_discernible_value: i64, highest_trackable_value: i64, significant_figures: i32, result: **hdr_histogram!?!): i32;`
- `export fn hdr_close(h: *hdr_histogram!);`
- `export fn hdr_reset(h: *hdr_histogram!);`
- `export fn hdr_get_memory_size(h: *hdr_histogram!): usize;`
- `export fn hdr_record_value(h: *hdr_histogram!, value: i64): bool;`
- `export fn hdr_record_values(h: *hdr_histogram!, value: i64, count: i64): bool;`
- `export fn hdr_record_corrected_value(h: *hdr_histogram!, value: i64, expected_interval: i64): bool;`
- `export fn hdr_record_corrected_values(h: *hdr_histogram!, value: i64, count: i64, expected_interval: i64): bool;`
- `export fn hdr_add(h: *hdr_histogram!, from: *&hdr_histogram!): i64;`
- `export fn hdr_add_while_correcting_for_coordinated_omission(h: *hdr_histogram!, from: *hdr_histogram!, expected_interval: i64): i64;`
- `export fn hdr_min(h: *&hdr_histogram!): i64;`
- `export fn hdr_max(h: *&hdr_histogram!): i64;`
- `export fn hdr_mean(h: *&hdr_histogram!): f64;`
- `export fn hdr_stddev(h: *&hdr_histogram!): f64;`
- `export fn hdr_value_at_percentile(h: *&hdr_histogram!, percentile: f64): i64;`
- `export fn hdr_count_at_value(h: *&hdr_histogram!, value: i64): i64;`
- `export fn hdr_values_are_equivalent(h: *&hdr_histogram!, a: i64, b: i64): bool;`
- `export fn hdr_lowest_equivalent_value(h: *&hdr_histogram!, value: i64): i64;`
- `export fn hdr_median_equivalent_value(h: *&hdr_histogram!, value: i64): i64;`
- `export fn hdr_next_non_equivalent_value(h: *&hdr_histogram!, value: i64): i64;`
- `export fn hdr_size_of_equivalent_value_range(h: *&hdr_histogram!, value: i64): i64;`
- `export fn hdrClampF64(v: f64, lo: f64, hi: f64): f64;`
- `export fn hdrTotalCount(h: *&hdr_histogram!): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`hdrhistogram@1.0.0`)，许可证：MIT
