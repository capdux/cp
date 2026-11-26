# @cap/strum-enum

Clean-room MIT, zero-heap enum variant string registry, discriminant lookup, and case-insensitive parser for CP — modeled on Rust's `strum` crate (900M+ downloads).

## Dependencies
- `@cap/unicase` (`1.0.0`) — ASCII case-insensitive enum parsing (`fromStrIgnoreCase`).

## API
- `EnumRegistry` (`new`, `register`, `variantCount`, `fromStr`, `fromStrIgnoreCase`, `asStr`, `getMessage`, `variantAt`, `formatVariantList`)
