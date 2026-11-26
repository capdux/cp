# @cap/cargo-platform

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cargo-platform` ([https://github.com/rust-lang/cargo](https://github.com/rust-lang/cargo))

Zero-heap-allocation LLVM/Rust target triple parser and Cargo `cfg(...)` predicate evaluator in pure CP (inspired by Rust `cargo-platform`).

## Features

- `parseTargetTriple(triple: string): TargetTriple`: zero-copy span parser for 3-part and 4-part LLVM/Rust target triples
- `evalCfgExpr(expr: string, target: TargetEnv): CfgEvalResult`: recursive-descent evaluator supporting `cfg(...)`, `all(...)`, `any(...)`, `not(...)`, bare flags (`unix`, `windows`), and key-value predicates (`target_os`, `target_arch`, `target_env`, `target_family`, `target_pointer_width`, `target_endian`)

## License

MIT
