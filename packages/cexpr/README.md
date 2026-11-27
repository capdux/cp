[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# cexpr

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cexpr` ([https://github.com/jethrogb/rust-cexpr](https://github.com/jethrogb/rust-cexpr))

Zero-heap-allocation C constant expression parser and evaluator in pure CP (inspired by Rust `cexpr`).

## Features

- Full C operator precedence: `(...)`, unary `+ - ~ !`, `* / %`, `+ -`, `<< >>`, `< <= > >=`, `== !=`, `&`, `^`, `|`, `&&`, `||`, ternary `cond ? a : b`
- Literal formats: decimal, hexadecimal (`0xFF`), binary (`0b1010`), octal (`0755`), character literals (`'A'`, `'\n'`, `'\t'`, `'\\'`, `'\0'`), and optional integer suffixes (`u`/`U`/`l`/`L`/`ul`/`ull`)
- Safe error reporting (`EvalResult { value, ok }`) for division by zero, unclosed parens, invalid octal digits, excessive nesting, and trailing tokens

## License

MIT
