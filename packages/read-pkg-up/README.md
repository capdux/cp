[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/read-pkg-up

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:read-pkg-up` ([https://github.com/sindresorhus/read-pkg-up](https://github.com/sindresorhus/read-pkg-up))

Zero-allocation, Clean-room MIT implementation of `read-pkg-up` v11 core semantics for CP, reusing `@cap/find-up` and `@cap/read-pkg`.

## Features

- **`PkgFileEntry`**: `{ path: string, content: string }` virtual entry representing a `package.json` path and its raw JSON text.
- **`ReadPkgUpResult`**: `{ found: bool, pkg_path_len: usize, pkg: ReadPkgResult }`.
- **`readPackageUpInEntries(cwd: string, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult`**: Uses `FindUpWalker` from `@cap/find-up` to walk upward from `cwd` (halting inclusively at `stop_at`), writes the matched `package.json` path into `path_out`, and parses/normalizes its metadata via `parsePackageJson` from `@cap/read-pkg`.

## Dependencies

- `@cap/find-up` (`1.0.0`)
- `@cap/read-pkg` (`1.0.0`)

## License

MIT
