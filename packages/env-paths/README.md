# @cap/env-paths

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:env-paths` ([https://github.com/sindresorhus/env-paths](https://github.com/sindresorhus/env-paths), `MIT`)

Zero-heap-allocation cross-platform application directory resolver in pure CP, modeled after Sindre Sorhus's `env-paths` v3.

## Features

- `EnvPlatform`: `Linux`, `MacOS`, `Windows`
- `EnvPathsInput`: holds `platform`, `home`, `xdg_data_home`, `xdg_config_home`, `xdg_cache_home`, `xdg_state_home`, `tmpdir`, `local_app_data`, `app_data`, and `suffix` (default `"-cp"`)
- Directory resolvers writing directly into caller-provided `[]u8`:
  - `envDataDir(name, inp, out): usize`
  - `envConfigDir(name, inp, out): usize`
  - `envCacheDir(name, inp, out): usize`
  - `envLogDir(name, inp, out): usize`
  - `envTempDir(name, inp, out): usize`
- Follows XDG Base Directory Specification (Linux), `~/Library/...` (macOS), and `%LOCALAPPDATA%` / `%APPDATA%` (Windows)

## License

MIT
