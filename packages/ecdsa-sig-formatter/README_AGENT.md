# ecdsa-sig-formatter@1.0.0 (Agent Quickref)

> Zero-allocation ASN.1 DER <-> JOSE IEEE P1363 (r || s) ECDSA signature converter for ES256, ES384, and ES512 in pure CP | License: Apache-2.0
> Import: `import { ... } from "ecdsa-sig-formatter";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum EcdsaAlg`

## Functions
- `export fn paramBytes(alg: EcdsaAlg): usize;`
- `export fn esfParseDerInt(der: &[]u8, pos: usize, limit: usize, param_len: usize, dst: []u8): i64;`
- `export fn esfTrimLeadingZeros(src: &[]u8): usize;`
- `export fn derToJose(der: &[]u8, alg: EcdsaAlg, out: []u8): i64;`
- `export fn joseToDer(jose: &[]u8, alg: EcdsaAlg, out: []u8): i64;`
