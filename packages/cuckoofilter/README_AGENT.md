# cuckoofilter@1.0.0 (Agent Quickref)

> Zero-allocation CoNEXT'14 Cuckoo Filter (256x4 16-bit fingerprint slots) with O(1) insert, contains, and safe delete for CP | License: MIT
> Import: `import { ... } from "cuckoofilter";` | Full types: `src/index.d.cp`

## Functions
- `export fn mulWrap64(a: u64, b: u64): u64;`
- `export fn avalanche64(x: u64): u64;`
- `export fn hashBytes(data: &[]u8): u64;`
- `export fn fingerprintFromHash(h: u64): u16;`
- `export fn altBucket(bucket: usize, fp: u16): usize;`
