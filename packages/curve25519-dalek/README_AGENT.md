# curve25519-dalek@1.0.0 (Agent Quickref)

> Zero-allocation constant-time RFC 7748 X25519 elliptic curve Diffie-Hellman and GF(2^255-19) field arithmetic (curve25519-dalek parity) for CP | License: BSD-3-Clause
> Import: `import { ... } from "curve25519-dalek";` | Full types: `src/index.d.cp`

## Functions
- `export fn c25Carry(o: []i64);`
- `export fn c25Sel(p: []i64, q: []i64, b: i64);`
- `export fn clampScalar(k: [32]u8): [32]u8;`
- `export fn x25519(scalar: [32]u8, u_coord: [32]u8): [32]u8;`
- `export fn x25519Basepoint(scalar: [32]u8): [32]u8;`
