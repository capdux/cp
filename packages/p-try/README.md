[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# p-try

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-try` ([https://github.com/sindresorhus/p-try](https://github.com/sindresorhus/p-try))

Clean-room MIT, zero-heap structured execution outcome wrapper for CP — modeled on npm's `p-try` v3. Wraps fallible operations into a uniform `TryOutcome` value so synchronous faults never escape as panics.

## API
- `TryOutcome` (`ok`, `value_i64`, `value_str`, `err_code`, `err_msg`, `okInt`, `okStr`, `err`, `isOk`, `isErr`)
- `tryOkInt(v: i64): TryOutcome`
- `tryOkStr(s: string): TryOutcome`
- `tryErr(code: i32, msg: string): TryOutcome`
- `unwrapOrInt(o: TryOutcome, fallback: i64): i64`
- `unwrapOrStr(o: TryOutcome, fallback: string): string`
- `tryParseI64(s: string): TryOutcome`
- `tryDivI64(a: i64, b: i64): TryOutcome`
- `tryChain(first: TryOutcome, next_ok: bool, next_val: i64, next_err_code: i32, next_err_msg: string): TryOutcome`
