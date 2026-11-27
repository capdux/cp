# hdrhistogram@1.0.0 (Agent Quickref)

> CP package hdrhistogram | License: BSD-2-Clause
> Import: `import { ... } from "hdrhistogram";` | Full types: `src/index.d.cp`

## Functions
- `export fn hdr_init(lowest_discernible_value: i64, highest_trackable_value: i64, significant_figures: i32, result: **hdr_histogram!?!): i32;`
- `export fn hdr_close(h: *hdr_histogram!);`
- `export fn hdr_reset(h: *hdr_histogram!);`
- `export fn hdr_get_memory_size(h: *hdr_histogram!): usize;`
- `export fn hdr_record_value(h: *hdr_histogram!, value: i64): bool;`
- `export fn hdr_record_values(h: *hdr_histogram!, value: i64, count: i64): bool;`
- `export fn hdr_record_corrected_value(h: *hdr_histogram!, value: i64, expected_interval: i64): bool;`
- `export fn hdr_min(h: *&hdr_histogram!): i64;`
- `export fn hdr_max(h: *&hdr_histogram!): i64;`
- `export fn hdr_mean(h: *&hdr_histogram!): f64;`
- `export fn hdr_stddev(h: *&hdr_histogram!): f64;`
- `export fn hdr_value_at_percentile(h: *&hdr_histogram!, percentile: f64): i64;`
