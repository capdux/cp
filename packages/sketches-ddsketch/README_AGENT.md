# sketches-ddsketch@0.3.0 (Agent Quickref)

> Zero-heap DDSketch relative-accuracy quantile sketch for low-memory latency statistics | License: Apache-2.0
> Import: `import { ... } from "sketches-ddsketch";` | Full types: `src/index.d.cp`

## Functions
- `export fn ddskZeroBuckets(): [DDSK_BUCKETS]u64;`
- `export fn ddskClampAlpha(alpha: f64): f64;`
- `export fn ddskAddKeyCount(sk: &mut DDSketch, key: i32, cnt: u64);`
