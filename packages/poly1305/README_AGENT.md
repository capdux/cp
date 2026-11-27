# poly1305@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 8439 Poly1305 one-time authenticator (Donna 26-bit limbs + constant-time verification via @cap/subtle-ct) | License: MIT
> Import: `import { ... } from "poly1305";` | Full types: `src/index.d.cp`
> Deps: `subtle`

## Functions
- `export fn poly1305Mac(key32: []u8, msg: []u8, out_tag16: []u8): bool;`
- `export fn poly1305Verify(key32: []u8, msg: []u8, expected_tag16: []u8): bool;`
