<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`nopt`](https://github.com/npm/nopt)（`ISC`）（包名 `@cap/nopt`）。

# `@cap/nopt`

Zero-allocation CLI option parser in pure CP (clean-room ISC implementation inspired by npm `nopt`).

## Features

- **`NoptFlag` / `NoptShorthand` / `NoptResult`** — Fixed-capacity stack-allocated option and positional argument storage.
- **`parseNopt(args, shorthands)`** — Supports `--key=val`, `--key val`, `--flag`, `--no-flag`, `-s` shorthand expansion (including bundled `-abc`), and `--` option terminator.
- **Query API** — `getString(key)`, `getBool(key)`, `has(key)`, `remainCount()`, `remainAt(idx)`.

## Usage

```cp
import { NoptShorthand, NoptResult, parseNopt } from "@cap/nopt";
```

## License

ISC
