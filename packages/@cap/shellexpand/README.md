<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:shellexpand` ([https://gitlab.com/ijackson/rust-shellexpand](https://gitlab.com/ijackson/rust-shellexpand)) | 许可：`MIT OR Apache-2.0`

# `@cap/shellexpand`

零堆分配纯 CP Shell 风格波浪线（`~`）与环境变量（`$VAR` / `${VAR}` / `${VAR:-default}` / `$$`）展开引擎，对标 crates.io 高下载库 `shellexpand` v3。

## 核心 API

- `tildeWithHome(input: string, home_dir: string, out: []u8): usize` — 展开前导 `~` / `~/...`，保留 `~other/...`
- `envExpand(input: string, vars: []const EnvEntry, keep_unknown: bool, out: []u8): usize` — 展开 `$VAR`、`${VAR}`、`${VAR:-default}` 与转义 `$$`
- `fullExpand(input: string, home_dir: string, vars: []const EnvEntry, out: []u8): usize` — 组合波浪线 + 环境变量展开
