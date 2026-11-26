# @cap/cuid2

Clean-room MIT, zero-heap collision-resistant CUID2 ID generator & validator for CP — modeled on `@paralleldrive/cuid2`.

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — fingerprint & counter entropy mixing.

## API
- Constants: `CUID2_DEFAULT_LEN (24)`, `CUID2_MIN_LEN (2)`, `CUID2_MAX_LEN (32)`
- `isCuid2(id): bool` & `isCuid2WithBounds(id, min_len, max_len): bool`
- `Cuid2Generator` (`new`, `length`, `next`)
