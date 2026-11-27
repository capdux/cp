[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/tinybench

Zero-heap micro-benchmark statistics engine in pure CP computing `mean_ns`, `ops_per_sec`, `min_ns`, `max_ns`, `variance`, `stddev_ns`, `p50`/`p75`/`p95`/`p99`, and 95% CI relative margin of error (`rme_percent`).

## API

- `computeStats(samples_ns: []const f64): BenchStats`
- `speedupRatio(baseline: BenchStats, candidate: BenchStats): f64`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`tinybench@3.2.0`)
- **Upstream Repository**: https://github.com/tinylibs/tinybench
- **License**: MIT (clean-room CP implementation)
