[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/predicates

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:predicates` ([https://github.com/assert-rs/predicates-rs](https://github.com/assert-rs/predicates-rs)) — `MIT OR Apache-2.0`

Zero-heap composable boolean predicates for assertions and filtering in pure CP (inspired by Rust `assert-rs/predicates`).

## Features

- `IntPredicate`: `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `inRange`, `always`, `never` + composable `.andPred`, `.orPred`, `.notPred`, `.eval(x: i64)`
- `StrPredicate`: `eq`, `contains`, `hasPrefix`, `hasSuffix`, `isEmpty`, `lenEq`, `lenBetween`, `eqIgnoreCase`, `always`, `never` + composable `.andPred`, `.orPred`, `.notPred`, `.eval(s: string)`
- `FloatPredicate`: `approxEq(target, epsilon)`, `inRange(lo, hi)`, `isFinite()`, `always`, `never` + composable `.andPred`, `.orPred`, `.notPred`, `.eval(x: f64)`

## License

MIT OR Apache-2.0
