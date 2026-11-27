# rfc6979@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 6979 deterministic ECDSA nonce generator (HMAC-SHA256 DRBG, RustCrypto rfc6979 parity) | License: MIT
> Import: `import { ... } from "rfc6979";` | Full types: `src/index.d.cp`
> Deps: `cookie-signature`

## Functions
- `export fn generateK(priv_key_32: []u8, msg_hash_32: []u8, order_32: []u8, out_k32: []u8): bool;`
