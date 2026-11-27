# character-entities@2.0.2 (Agent Quickref)

> Zero-allocation HTML5 named and numeric character reference decoder and encoder (covers decode-named-character-reference, parse-entities, stringify-entities) | License: MIT
> Import: `import { ... } from "character-entities";` | Full types: `src/index.d.cp`

## Functions
- `export fn mapInvalidCharRef(cp: u32): u32;`
- `export fn decodeNamedEntity(name: string): u32?;`
- `export fn decodeNumericEntity(body: string): u32?;`
- `export fn decodeEntities(input: string, out: []u8): usize;`
- `export fn encodeEntities(input: string, out: []u8): usize;`
