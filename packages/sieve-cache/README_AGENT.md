# sieve-cache@1.0.0 (Agent Quickref)

> Zero-allocation NSDI'24 SIEVE cache eviction engine with static doubly-linked pool and open-addressing index for CP | License: MIT
> Import: `import { ... } from "sieve-cache";` | Full types: `src/index.d.cp`

## Functions
- `export fn emptyNode(): SieveNode;`
- `export fn mulWrap64(a: u64, b: u64): u64;`
- `export fn hashSlot(key: u64): usize;`
