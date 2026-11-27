# tiny-keccak@1.0.0 (Agent Quickref)

> Zero-allocation Keccak-f[1600] sponge with SHA3-256 (FIPS 202) and Keccak-256 (Ethereum) (Rust tiny-keccak parity) for CP | License: MIT
> Import: `import { ... } from "tiny-keccak";` | Full types: `src/index.d.cp`

## Functions
- `export fn kckU64(hi: u32, lo: u32): u64;`
- `export fn kckRotl64(x: u64, n: u32): u64;`
- `export fn kckReadLe64(b: []u8, off: usize): u64;`
- `export fn kckWriteLe64(out: []u8, off: usize, v: u64);`
- `export fn kckHexDigit(nibble: u8): u8;`
- `export fn kckRoundConst(round: usize): u64;`
- `export fn kckPermute(st: []u64);`
- `export fn kckAbsorbBlock(st: []u64, block: []u8);`
- `export fn kckFinalizeSponge(st: []u64, buf: []u8, buf_len: usize, domain_sep: u8, out32: []u8);`
- `export fn kckEncodeHex32(digest: []u8, out64: []u8): usize;`
- `export fn sha3_256Hex(s: string, out64: []u8): usize;`
- `export fn keccak256Hex(s: string, out64: []u8): usize;`
