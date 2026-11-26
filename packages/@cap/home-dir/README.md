# `@cap/home-dir`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:home` ([https://github.com/rust-lang/cargo](https://github.com/rust-lang/cargo)) / `npm:untildify` ([https://github.com/sindresorhus/untildify](https://github.com/sindresorhus/untildify)) / `npm:expand-tilde` ([https://github.com/jonschlinkert/expand-tilde](https://github.com/jonschlinkert/expand-tilde)) / `npm:os-homedir` ([https://github.com/sindresorhus/os-homedir](https://github.com/sindresorhus/os-homedir)) / `crates.io:dirs` ([https://github.com/dirs-dev/dirs-rs](https://github.com/dirs-dev/dirs-rs))（原库名 `home` → CP 包名 `@cap/home-dir`）

Zero-heap cross-platform home directory resolver, tilde expansion/contraction, and XDG config directory builder in pure CP (clean-room counterpart to Rust `home` + npm `untildify`/`tildify`).

## Features

- **100% Zero Heap Allocation & Pure Logic**: No syscalls — operates directly on passed environment strings and caller-provided `out: []u8`.
- **Cross-Platform Resolution**: `resolveHomeDir` implements Rust `home` priority (POSIX `HOME`; Windows `HOME` -> `USERPROFILE` -> `HOMEDRIVE + HOMEPATH`).
- **Safe Tilde Transforms**: `expandTilde` (`~`, `~/...`, `~\...`, leaving `~bob/...` untouched) and `contractTilde` (with path-segment boundary guard).
- **XDG Config Paths**: `joinConfigDir` supports `XDG_CONFIG_HOME` override, POSIX `~/.config/<app>`, and Windows `AppData\Roaming\<app>`.

## Quick Start

```cp
import { resolveHomeDir, expandTilde, contractTilde, joinConfigDir } from "@cap/home-dir";

let buf: [128]u8 = undefined;
const n1 = expandTilde("~/.cap/cache", "/home/alice", &buf); // "/home/alice/.cap/cache"
const n2 = contractTilde("/home/alice/docs", "/home/alice", &buf); // "~/docs"
```

## License

MIT
