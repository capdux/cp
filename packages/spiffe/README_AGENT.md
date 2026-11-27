# spiffe@1.0.0 (Agent Quickref)

> Zero-allocation CNCF SPIFFE ID (spiffe://<trust-domain>/<path>) parser, strict security validator, and formatter for CP. | License: MIT
> Import: `import { ... } from "spiffe";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidTrustDomain(td: string): bool;`
- `export fn isValidPath(path: string): bool;`
- `export fn parseSpiffeId(uri: string): SpiffeId;`
- `export fn belongsToTrustDomain(id: SpiffeId, trust_domain: string): bool;`
- `export fn formatSpiffeId(trust_domain: string, path: string, out: []u8): usize;`
