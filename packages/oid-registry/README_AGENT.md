# oid-registry@1.0.0 (Agent Quickref)

> Zero-allocation static X.509/PKCS/TLS/NIST/RFC OID registry with DER OID encoder and decoder (Rust oid-registry parity) for CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "oid-registry";` | Full types: `src/index.d.cp`

## Functions
- `export fn oidrEntryAt(idx: usize): OidEntry;`
- `export fn oidrStrEq(a: string, b: string): bool;`
- `export fn oidEntryCount(): usize;`
- `export fn lookupOid(dotted: string): OidEntry?;`
- `export fn lookupShortName(sn: string): OidEntry?;`
- `export fn oidrEncodeBase128(val: u64, out: []u8, pos: usize): usize?;`
- `export fn oidrWriteU64Dec(val: u64, out: []u8, pos: usize): usize?;`
- `export fn encodeOidDer(dotted: string, out: []u8): usize;`
- `export fn oidrDecodeBase128(der: &[]u8, start: usize): OidrDecodeStep;`
- `export fn decodeOidDer(der: &[]u8, out: []u8): usize;`
