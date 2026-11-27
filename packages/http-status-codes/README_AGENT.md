# http-status-codes@1.0.0 (Agent Quickref)

> Zero-allocation RFC 9110, WebDAV, and Cloudflare HTTP status code constants, bidirectional phrase lookup, and semantic predicates for CP | License: MIT
> Import: `import { ... } from "http-status-codes";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum StatusClass`

## Functions
- `export fn reasonPhrase(code: u16): string;`
- `export fn isSep(c: u8): bool;`
- `export fn eqNormalizedPhrase(input: string, target: string): bool;`
- `export fn codeFromPhrase(phrase: string): u16?;`
- `export fn statusClass(code: u16): StatusClass;`
- `export fn isValidStatus(code: u16): bool;`
- `export fn isKnownStatus(code: u16): bool;`
- `export fn isInformational(code: u16): bool;`
- `export fn isSuccess(code: u16): bool;`
- `export fn isRedirect(code: u16): bool;`
- `export fn isClientError(code: u16): bool;`
- `export fn isServerError(code: u16): bool;`
- `export fn isEmptyBody(code: u16): bool;`
- `export fn isRetryable(code: u16): bool;`
- `export fn isCacheableByDefault(code: u16): bool;`
