# @cap/sketches-ddsketch

Zero-heap DDSketch relative-accuracy quantile sketch (`DDSketch`) with 512 logarithmic collapsing buckets for low-memory latency statistics in pure CP.

## API

- `DDSK_BUCKETS: usize` (`512`)
- `DDSketch.new(relative_accuracy: f64): DDSketch`
- `add(v: f64)` — record a sample
- `count(): u64` / `sum(): f64` / `min(): f64` / `max(): f64`
- `quantile(q: f64): f64` — estimate quantile (e.g. p50, p95, p99) within relative error `alpha`
- `merge(other: DDSketch)` — merge another sketch into this sketch

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`sketches-ddsketch@0.3.0`)
- **Upstream Repository**: https://github.com/DataDog/sketches-ddsketch-rust
- **License**: Apache-2.0 (clean-room CP implementation)
