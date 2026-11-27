# hyperloglog@1.0.0 (Agent Quickref)

> Zero-allocation 256-byte HyperLogLog cardinality estimator with LinearCounting small-range correction and distributed merge (Rust hyperloglog / Redis PFADD/PFCOUNT/PFMERGE parity) for CP | License: MIT
> Import: `import { ... } from "hyperloglog";` | Full types: `src/index.d.cp`

## Functions
- `export fn hashBytes64(data: string): u64;`
- `export fn hashU64(val: u64): u64;`
- `export fn clz56(w: u64): u8;`
