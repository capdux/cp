# lru-cache@1.0.0 (Agent Quickref)

> Zero-allocation O(1) LRU cache with intrusive doubly-linked list, open-addressing FNV-1a hash index, TTL expiration, and byte-weight eviction for CP | License: MIT
> Import: `import { ... } from "lru-cache";` | Full types: `src/index.d.cp`

## Functions
- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn bytesEqual(a: &[]u8, b: &[]u8): bool;`
- `export fn copyBytes(dst: []u8, src: &[]u8);`
