[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# getopts

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:getopts` ([https://github.com/rust-lang/getopts](https://github.com/rust-lang/getopts), `MIT OR Apache-2.0`)

Zero-heap-allocation POSIX/GNU command-line option parser in pure CP (inspired by Rust's official `getopts` crate).

## Features

- Enums: `HasArg` (`No`, `Yes`, `Maybe`), `Occur` (`Optional`, `Req`, `Multi`)
- `Options` (up to 32 option definitions): `new()`, `optflag`, `optflagmulti`, `optopt`, `optmulti`, `reqopt`, `opt`, `parse(args: []const string): Matches`
- `Matches`: `ok`, `err_kind`, `optPresent(name)`, `optCount(name)`, `optStr(name)`, `optStrs(name, out)`, `freeCount()`, `freeAt(idx)`
- Full support for clustered short flags (`-abc`), attached short option values (`-ofile`), `--long=value`, `--` positional separator, duplicate checks, and required-option validation

## License

MIT OR Apache-2.0
