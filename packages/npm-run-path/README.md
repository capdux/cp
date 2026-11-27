[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `npm-run-path`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:npm-run-path` ([https://github.com/sindresorhus/npm-run-path](https://github.com/sindresorhus/npm-run-path))

Zero-heap `PATH` builder that prepends ancestor `node_modules/.bin` (or `cap_modules/.bin`) directories for locally installed binaries — clean-room CP equivalent of npm `npm-run-path`.

## Features
- `RunPathOptions.defaults(cwd, path_env)` & `RunPathOptions.forCap(cwd, path_env)`
- `buildRunPath(opts, out)` — walks upward from `cwd` to root, prepending local `.bin` dirs, optional `exec_path` dir, and original `PATH`
- POSIX (`:` / `/`) and Windows (`;` / `\`) delimiter & separator normalization
- `dirnameOf`, `countAncestorDirs`, `pathKey`
