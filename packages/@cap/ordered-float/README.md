# @cap/ordered-float

Clean-room MIT, zero-heap IEEE-754 total ordering, canonical hashing, `OrderedF64`, `NotNanF64`, and `sortOrderedF64` for CP — modeled on crates.io #85 `ordered-float` (1.2B+ downloads).

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — canonical bit-pattern hashing.

## API
- `OrderedF64` (`new`, `intoInner`, `isNan`, `isFinite`, `cmp`, `eq`, `hash`, `min`, `max`, `clamp`)
- `NotNanF64` (`new`, `intoInner`, `cmp`, `eq`, `hash`)
- `cmpOrderedF64`, `eqOrderedF64`, `hashOrderedF64`, `isNanF64`, `isInfiniteF64`, `isFiniteF64`, `sortOrderedF64`
