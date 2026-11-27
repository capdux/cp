# pbkdf2@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256 password-based key derivation and constant-time verification | License: MIT
> Import: `import { ... } from "pbkdf2";` | Full types: `src/index.d.cp`
> Deps: `cookie-signature`

## Functions
- `export fn pbkdf2HmacSha256(password: []u8, salt: []u8, iterations: u32, out_dk: []u8): bool;`
- `export fn pbkdf2Verify(password: []u8, salt: []u8, iterations: u32, expected_dk: []u8): bool;`
