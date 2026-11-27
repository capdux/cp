# hashring@1.0.0 (Agent Quickref)

> Zero-allocation consistent hash ring with virtual nodes, O(log V) binary search routing, and deduplicated replica selection for CP | License: MIT
> Import: `import { ... } from "hashring";` | Full types: `src/index.d.cp`

## Functions
- `export fn mulWrap64(a: u64, b: u64): u64;`
- `export fn avalanche64(x: u64): u64;`
- `export fn fnv1a64Feed(state: u64, b: u8): u64;`
- `export fn hashKey(key: string): u64;`
- `export fn hashVnode(name: string, v_idx: u16): u64;`
- `export fn eqStr(a: string, b: string): bool;`
