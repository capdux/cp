# serde_qs@1.0.0 (Agent Quickref)

> Zero-allocation nested bracket query string parser & serializer (crates.io serde_qs v0.13 parity) composing @cap/percent-encoding for CP. | License: MIT
> Import: `import { ... } from "serde_qs";` | Full types: `src/index.d.cp`
> Deps: `percent-encoding`

## Functions
- `export fn parseKeyPath(key: string): QsPath;`
- `export fn parseQs(query: string, out_pairs: []QsPair): usize;`
- `export fn getQsValueDecoded(query: string, target_key: string, out: []u8): usize?;`
- `export fn serializeQs(pairs: &[]QsPair, out: []u8): usize;`
