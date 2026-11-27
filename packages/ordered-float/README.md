[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ordered-float

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ordered-float` ([https://github.com/reem/rust-ordered-float](https://github.com/reem/rust-ordered-float))

Clean-room MIT, zero-heap IEEE-754 total ordering, canonical hashing, `OrderedF64`, `NotNanF64`, and `sortOrderedF64` for CP — modeled on crates.io #85 `ordered-float` (1.2B+ downloads).

## Dependencies
- `@sindresorhus/fnv1a` (`1.0.0`) — canonical bit-pattern hashing.

## API
- `OrderedF64` (`new`, `intoInner`, `isNan`, `isFinite`, `cmp`, `eq`, `hash`, `min`, `max`, `clamp`)
- `NotNanF64` (`new`, `intoInner`, `cmp`, `eq`, `hash`)
- `cmpOrderedF64`, `eqOrderedF64`, `hashOrderedF64`, `isNanF64`, `isInfiniteF64`, `isFiniteF64`, `sortOrderedF64`
